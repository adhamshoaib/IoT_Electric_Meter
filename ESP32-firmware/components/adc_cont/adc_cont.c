#include "adc_cont.h"

#include "esp_log.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

#define TAG "ADC_DRV"

// ─── private state ────────────────────────────────────────────────────────────

static adc_continuous_handle_t s_handle      = NULL;
static adc_cali_handle_t       s_cali_handle = NULL;
static TaskHandle_t            s_task_handle = NULL;
static adc_drv_cb_t            s_callback    = NULL;

// Task working buffers at file scope — guarantees BSS placement and makes
// their size visible to the linker map. Keeps the task stack lean so only
// the call-chain frames (adc_continuous_read, parse, cali) consume stack.
static uint8_t               s_raw_buf[ADC_DRV_FRAME_SIZE];
static adc_continuous_data_t s_parsed[ADC_DRV_MAX_SAMPLES];
static adc_drv_sample_t      s_clean[ADC_DRV_MAX_SAMPLES];

// ─── ISR callback ─────────────────────────────────────────────────────────────

/**
 * Runs in ISR context — only unblocks the processing task.
 * No ADC reads or heap allocations here.
 */
static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t  handle,
                                     const adc_continuous_evt_data_t *edata,
                                     void                    *user_data)
{
    BaseType_t higher_prio_task_woken = pdFALSE;
    vTaskNotifyGiveFromISR(s_task_handle, &higher_prio_task_woken);
    return (higher_prio_task_woken == pdTRUE);
}

// ─── calibration helpers ──────────────────────────────────────────────────────

/**
 * Convert one raw ADC reading to millivolts.
 * Falls back to a direct cast when calibration is unavailable so the
 * callback always receives a meaningful float (callers must be aware that
 * without calibration the value is in raw counts, not mV).
 */
static inline float s_to_mv(int raw)
{
    if (s_cali_handle) {
        int mv = 0;
        adc_cali_raw_to_voltage(s_cali_handle, raw, &mv);
        return (float)mv;
    }
    return (float)raw;
}

// ─── processing task ──────────────────────────────────────────────────────────

static void adc_drv_task(void *arg)
{
    while (1) {
        // Block until the ISR signals a completed conversion frame.
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        uint32_t  bytes_read = 0;
        esp_err_t ret = adc_continuous_read(s_handle,
                                            s_raw_buf, ADC_DRV_FRAME_SIZE,
                                            &bytes_read, 0);

        if (ret == ESP_ERR_TIMEOUT) {
            // No frame ready yet — spurious wake, ignore.
            continue;
        }

        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "adc_continuous_read error: 0x%x", ret);
            continue;
        }

        if (bytes_read == 0) {
            continue;
        }

        // ── parse raw DMA bytes into typed structs ──
        uint32_t num_parsed = 0;
        ret = adc_continuous_parse_data(s_handle,
                                        s_raw_buf, bytes_read,
                                        s_parsed, &num_parsed);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "adc_continuous_parse_data error: 0x%x", ret);
            continue;
        }

        // ── calibrate and build clean output array ──
        uint32_t num_clean = 0;
        for (uint32_t i = 0; i < num_parsed; i++) {
            if (!s_parsed[i].valid) {
                continue;
            }
            s_clean[num_clean].channel    = (adc_channel_t)s_parsed[i].channel;
            s_clean[num_clean].voltage_mv = s_to_mv(s_parsed[i].raw_data);
            num_clean++;
        }

        // ── deliver to sensor layer ──
        if (s_callback && num_clean > 0) {
            s_callback(s_clean, num_clean);
        }
    }
}

// ─── public API ───────────────────────────────────────────────────────────────

esp_err_t adc_drv_init(const adc_channel_t *channels,
                       uint8_t              num_channels,
                       adc_drv_cb_t         callback)
{
    // ── argument guards ──
    if (channels == NULL || num_channels == 0) {
        ESP_LOGE(TAG, "Invalid channel configuration");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_handle != NULL) {
        ESP_LOGE(TAG, "Driver already initialised — call adc_drv_deinit() first");
        return ESP_ERR_INVALID_STATE;
    }

    // ── all locals declared upfront — required when using goto ──
    esp_err_t ret;
    BaseType_t task_ret;

    // Fixed-size array — VLAs cannot coexist with goto in the same scope.
    adc_digi_pattern_config_t   pattern[SOC_ADC_PATT_LEN_MAX];
    adc_continuous_handle_cfg_t handle_cfg;
    adc_continuous_config_t     dig_cfg;
    adc_continuous_evt_cbs_t    cbs;
    adc_cali_line_fitting_config_t cali_cfg;

    s_callback = callback;

    // ── 1. Spawn processing task before registering ISR ──
    //    The ISR calls vTaskNotifyGiveFromISR(); s_task_handle must be valid.
    task_ret = xTaskCreate(adc_drv_task,
                           "adc_drv_task",
                           ADC_DRV_TASK_STACK,
                           NULL,
                           ADC_DRV_TASK_PRIORITY,
                           &s_task_handle);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create ADC task");
        return ESP_FAIL;
    }

    // ── 2. Create continuous handle ──
    handle_cfg.max_store_buf_size = ADC_DRV_BUF_SIZE;
    handle_cfg.conv_frame_size    = ADC_DRV_FRAME_SIZE;

    ret = adc_continuous_new_handle(&handle_cfg, &s_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_continuous_new_handle failed: 0x%x", ret);
        goto err_task;
    }

    // ── 3. Build channel pattern ──
    for (uint8_t i = 0; i < num_channels; i++) {
        pattern[i].atten     = ADC_ATTEN_DB_12;
        pattern[i].channel   = channels[i];
        pattern[i].unit      = ADC_UNIT_1;
        pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;
    }

    dig_cfg.sample_freq_hz = ADC_DRV_SAMPLE_FREQ_HZ;
    dig_cfg.conv_mode      = ADC_CONV_SINGLE_UNIT_1;
    dig_cfg.format         = ADC_DIGI_OUTPUT_FORMAT_TYPE2;
    dig_cfg.pattern_num    = num_channels;
    dig_cfg.adc_pattern    = pattern;

    ret = adc_continuous_config(s_handle, &dig_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_continuous_config failed: 0x%x", ret);
        goto err_handle;
    }

    // ── 4. Register conversion-done ISR ──
    cbs.on_conv_done = s_conv_done_cb;

    ret = adc_continuous_register_event_callbacks(s_handle, &cbs, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_continuous_register_event_callbacks failed: 0x%x", ret);
        goto err_handle;
    }

    // ── 5. Calibration (best-effort) ──
    cali_cfg.unit_id  = ADC_UNIT_1;
    cali_cfg.atten    = ADC_ATTEN_DB_12;
    cali_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;

    ret = adc_cali_create_scheme_line_fitting(&cali_cfg, &s_cali_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Calibration unavailable — raw counts will be forwarded as mV");
        s_cali_handle = NULL;
    }

    ESP_LOGI(TAG, "Initialised: %d channel(s), calibration %s",
             num_channels, s_cali_handle ? "ON" : "OFF");
    return ESP_OK;

    // ── cleanup on partial initialisation failure ──
err_handle:
    adc_continuous_deinit(s_handle);
    s_handle = NULL;
err_task:
    vTaskDelete(s_task_handle);
    s_task_handle = NULL;
    s_callback    = NULL;
    return ret;
}

esp_err_t adc_drv_start(void)
{
    if (s_handle == NULL) {
        ESP_LOGE(TAG, "Driver not initialised");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = adc_continuous_start(s_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_continuous_start failed: 0x%x", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Started");
    return ESP_OK;
}

esp_err_t adc_drv_stop(void)
{
    if (s_handle == NULL) {
        ESP_LOGE(TAG, "Driver not initialised");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = adc_continuous_stop(s_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_continuous_stop failed: 0x%x", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Stopped");
    return ESP_OK;
}

esp_err_t adc_drv_deinit(void)
{
    if (s_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret;

    // ── 1. Stop conversions (ignore already-stopped state) ──
    ret = adc_continuous_stop(s_handle);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "adc_continuous_stop failed: 0x%x", ret);
        return ret;
    }

    // ── 2. Release ADC handle ──
    ret = adc_continuous_deinit(s_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_continuous_deinit failed: 0x%x", ret);
        return ret;
    }
    s_handle = NULL;

    // ── 3. Release calibration ──
    if (s_cali_handle) {
        adc_cali_delete_scheme_line_fitting(s_cali_handle);
        s_cali_handle = NULL;
    }

    // ── 4. Delete processing task ──
    if (s_task_handle) {
        vTaskDelete(s_task_handle);
        s_task_handle = NULL;
    }

    s_callback = NULL;

    ESP_LOGI(TAG, "Deinitialized");
    return ESP_OK;
}