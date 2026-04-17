/**
 * @file adc_driver.c
 * @brief ADC driver implementation - see adc_driver.h for API documentation.
 */

#include "adc_driver.h"

#include <stdbool.h>
#include <stdlib.h>
#include "esp_check.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define ADC_SAMPLE_RATE_HZ 20000
#define CYCLES_PER_WINDOW 4
#define SAMPLES_PER_WINDOW (ADC_SAMPLE_RATE_HZ / 50 * CYCLES_PER_WINDOW)
#define ADC_FRAME_SIZE (SAMPLES_PER_WINDOW * sizeof(adc_digi_output_data_t))
#define ADC_RING_BUF_SIZE (ADC_FRAME_SIZE * 2)

typedef struct
{
    bool init;
    bool running;
    uint8_t *raw_buf;
    adc_continuous_handle_t handle;
    adc_cali_handle_t cali_handle;
    bool cali_enabled;
} adc_driver_t;

#define NUM_CHANNELS 2
static const adc_channel_t channels[NUM_CHANNELS] = {ADC_CHANNEL_6, ADC_CHANNEL_7};
#define VOLTAGE_CHANNEL_IDX 0
#define CURRENT_CHANNEL_IDX 1

static adc_driver_t adc_driver = {0};

static const char *TAG = "ADC_DRIVER";

static esp_err_t adc_driver_cali_init(void)
{
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,   // must match channel config
        .bitwidth = ADC_BITWIDTH_12 // must match channel config
        // .default_vref = 0 (optional; 0 => use eFuse)
    };
    ESP_RETURN_ON_ERROR(adc_cali_create_scheme_line_fitting(&cali_cfg, &adc_driver.cali_handle), TAG, "adc_cali_create_scheme_line_fitting failed");

    adc_driver.cali_enabled = true;
    return ESP_OK;
}

static esp_err_t adc_driver_raw_to_mv(uint16_t raw, int *mv_out)
{
    if (!adc_driver.cali_enabled || adc_driver.cali_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int mv = 0;
    esp_err_t ret = adc_cali_raw_to_voltage(adc_driver.cali_handle, raw, &mv);
    if (ret != ESP_OK)
    {
        return ret;
    }
    *mv_out = mv;
    return ESP_OK;
}

static void adc_driver_cali_deinit(void)
{
    if (!adc_driver.cali_enabled || adc_driver.cali_handle == NULL)
    {
        return;
    }

    esp_err_t ret = adc_cali_delete_scheme_line_fitting(adc_driver.cali_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "adc_cali_delete_scheme_line_fitting failed: %s", esp_err_to_name(ret));
    }

    adc_driver.cali_handle = NULL;
    adc_driver.cali_enabled = false;
}

esp_err_t adc_driver_init(void)
{
    if (adc_driver.init)
        return ESP_ERR_INVALID_STATE;

    adc_continuous_handle_cfg_t handle_config = {
        .max_store_buf_size = ADC_RING_BUF_SIZE,
        .conv_frame_size = ADC_FRAME_SIZE,
    };

    ESP_RETURN_ON_ERROR(adc_continuous_new_handle(&handle_config, &adc_driver.handle), TAG, "adc_continuous_new_handle failed");

    adc_continuous_config_t adc_config = {
        .pattern_num = NUM_CHANNELS,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
        .sample_freq_hz = ADC_SAMPLE_RATE_HZ,
    };

    adc_digi_pattern_config_t channel_config[2];
    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        channel_config[i].channel = channels[i];
        channel_config[i].atten = ADC_ATTEN_DB_12;
        channel_config[i].bit_width = ADC_BITWIDTH_12;
        channel_config[i].unit = ADC_UNIT_1;
    }
    adc_config.adc_pattern = channel_config;

    esp_err_t ret = adc_continuous_config(adc_driver.handle, &adc_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "adc_continuous_config failed: %s", esp_err_to_name(ret));
        goto cleanup_handle;
    }

    // calibration
    ret = adc_driver_cali_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "adc_driver_cali_init failed: %s", esp_err_to_name(ret));
        goto cleanup_handle;
    }

    adc_driver.raw_buf = malloc(ADC_FRAME_SIZE);
    if (adc_driver.raw_buf == NULL)
    {
        ret = ESP_ERR_NO_MEM;
        goto cleanup_handle;
    }

    adc_driver.init = true;
    adc_driver.running = false;

    return ESP_OK;

cleanup_handle:
    adc_driver_cali_deinit();
    adc_continuous_deinit(adc_driver.handle);
    if (adc_driver.raw_buf != NULL)
    {
        free(adc_driver.raw_buf);
        adc_driver.raw_buf = NULL;
    }
    adc_driver.handle = NULL;
    return ret;
}

esp_err_t adc_driver_deinit(void)
{
    if (!adc_driver.init)
        return ESP_ERR_INVALID_STATE;

    if (adc_driver.running)
    {
        esp_err_t stop_ret = adc_driver_stop();
        if (stop_ret != ESP_OK)
            ESP_LOGW(TAG, "adc_driver_stop failed during deinit: %s", esp_err_to_name(stop_ret));
    }

    esp_err_t ret = adc_continuous_deinit(adc_driver.handle);
    adc_driver_cali_deinit();

    adc_driver.handle = NULL;
    free(adc_driver.raw_buf);
    adc_driver.raw_buf = NULL;
    adc_driver.init = false;

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "adc_continuous_deinit failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "ADC driver deinitialized");
    return ESP_OK;
}

esp_err_t adc_driver_start(void)
{
    if (!adc_driver.init || adc_driver.running)
        return ESP_ERR_INVALID_STATE;

    ESP_RETURN_ON_ERROR(adc_continuous_start(adc_driver.handle), TAG, "adc_continuous_start failed");

    adc_driver.running = true;

    ESP_LOGI(TAG, "ADC driver started");

    return ESP_OK;
}

esp_err_t adc_driver_stop(void)
{
    if (!adc_driver.init || !adc_driver.running)
        return ESP_ERR_INVALID_STATE;

    ESP_RETURN_ON_ERROR(adc_continuous_stop(adc_driver.handle), TAG, "adc_continuous_stop failed");

    adc_driver.running = false;

    ESP_LOGI(TAG, "ADC driver stopped");

    return ESP_OK;
}

esp_err_t adc_driver_read(adc_sample_pair_t *out, size_t max_count, size_t *out_count, uint32_t timeout_ms)
{
    if (!adc_driver.init || !adc_driver.running)
        return ESP_ERR_INVALID_STATE;

    if (!out || !out_count || max_count == 0)
        return ESP_ERR_INVALID_ARG;

    uint32_t bytes_read = 0;

    esp_err_t ret = adc_continuous_read(adc_driver.handle, adc_driver.raw_buf, ADC_FRAME_SIZE, &bytes_read, timeout_ms);
    if (ret != ESP_OK)
        return ret;

    uint32_t frame_count = bytes_read / sizeof(adc_digi_output_data_t);

    size_t pairs_written = 0;
    bool pair_started = false;

    for (uint32_t i = 0; i < frame_count && pairs_written < max_count; i++)
    {
        adc_digi_output_data_t *frame = (adc_digi_output_data_t *)&adc_driver.raw_buf[i * sizeof(adc_digi_output_data_t)];
        uint8_t ch = frame->type1.channel;
        uint16_t data = frame->type1.data;

        if (ch == channels[VOLTAGE_CHANNEL_IDX])
        {
            int mv = 0;
            esp_err_t mv_ret = adc_driver_raw_to_mv(data, &mv);
            if (mv_ret != ESP_OK)
            {
                return mv_ret;
            }
            out[pairs_written].v_raw = data;
            out[pairs_written].v_mv = mv;
            pair_started = true;
        }

        else if (ch == channels[CURRENT_CHANNEL_IDX] && pair_started)
        {
            int mv = 0;
            esp_err_t mv_ret = adc_driver_raw_to_mv(data, &mv);
            if (mv_ret != ESP_OK)
            {
                return mv_ret;
            }
            out[pairs_written].i_raw = data;
            out[pairs_written].i_mv = mv;
            pairs_written++;
            pair_started = false;
        }
        else
        {
            ESP_LOGD(TAG, "Unexpected channel ID %d in DMA frame, skipping", ch);
        }
    }
    if (pairs_written == 0)
    {
        ESP_LOGW(TAG, "No complete pairs formed from %" PRIu32 " bytes read", bytes_read);
        return ESP_ERR_INVALID_RESPONSE;
    }

    *out_count = pairs_written;

    return ESP_OK;
}
