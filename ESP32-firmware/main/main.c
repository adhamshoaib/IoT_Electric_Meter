#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "adc_cont.h"
#include "voltage_sensor.h"
#include "current_sensor.h"

#define TAG "MAIN"

// ─── hardware config ──────────────────────────────────────────────────────────
// ZMPT101B → ADC_CHANNEL_6 (GPIO34)
// SCT013   → ADC_CHANNEL_7 (GPIO35)

#define VOLTAGE_SENSOR_CHANNEL   ADC_CHANNEL_6
#define CURRENT_SENSOR_CHANNEL   ADC_CHANNEL_7

static adc_channel_t s_channels[] = {
    VOLTAGE_SENSOR_CHANNEL,
    CURRENT_SENSOR_CHANNEL,
};
#define NUM_CHANNELS (sizeof(s_channels) / sizeof(s_channels[0]))

// ─── sensor contexts ──────────────────────────────────────────────────────────

static voltage_sensor_ctx_t *s_voltage_sensor = NULL;
static current_sensor_ctx_t *s_current_sensor = NULL;

// ─── raw ADC diagnostics (written from callback, read from task) ──────────────

static volatile float s_v_raw_min =  99999.0f;
static volatile float s_v_raw_max = -99999.0f;
static volatile float s_i_raw_min =  99999.0f;
static volatile float s_i_raw_max = -99999.0f;

// ─── ADC callback — fast path only ───────────────────────────────────────────

static void on_adc_samples(const adc_drv_sample_t *samples, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        float mv = samples[i].voltage_mv;

        if (samples[i].channel == VOLTAGE_SENSOR_CHANNEL) {
            if (mv < s_v_raw_min) s_v_raw_min = mv;
            if (mv > s_v_raw_max) s_v_raw_max = mv;
            voltage_sensor_feed(s_voltage_sensor, mv);
        }
        else if (samples[i].channel == CURRENT_SENSOR_CHANNEL) {
            if (mv < s_i_raw_min) s_i_raw_min = mv;
            if (mv > s_i_raw_max) s_i_raw_max = mv;
            current_sensor_feed(s_current_sensor, mv);
        }
    }
}

// ─── helpers ─────────────────────────────────────────────────────────────────

static void print_diagnostics(const char *name,
                               float raw_min, float raw_max,
                               float known_rms,
                               const char *scale_define)
{
    float swing_mv  = raw_max - raw_min;
    float midpoint  = (raw_max + raw_min) / 2.0f;
    float vpeak_adc = (swing_mv / 2.0f) / 1000.0f;
    float vrms_adc  = vpeak_adc / 1.41421f;

    ESP_LOGI(TAG, "  [%s] Min: %.1f mV | Max: %.1f mV | Swing: %.1f mV | Mid: %.1f mV",
             name, (double)raw_min, (double)raw_max,
             (double)swing_mv, (double)midpoint);
    ESP_LOGI(TAG, "  [%s] ADC Vrms: %.4f V  →  set %s = %.2f  (for %.0f RMS output)",
             name, (double)vrms_adc, scale_define,
             (double)(known_rms / vrms_adc), (double)known_rms);
    ESP_LOGI(TAG, "  [%s] Suggested DC offset: set define to %.0f mV",
             name, (double)midpoint);
}

// ─── test task ────────────────────────────────────────────────────────────────

static void test_task(void *arg)
{
    esp_err_t ret;

    // ── 1. Init sensors ──
    ret = voltage_sensor_init(VOLTAGE_SENSOR_CHANNEL, &s_voltage_sensor);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "voltage_sensor_init failed: 0x%x", ret);
        vTaskDelete(NULL);
        return;
    }

    ret = current_sensor_init(CURRENT_SENSOR_CHANNEL, &s_current_sensor);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "current_sensor_init failed: 0x%x", ret);
        voltage_sensor_deinit(s_voltage_sensor);
        vTaskDelete(NULL);
        return;
    }

    // ── 2. Init and start ADC ──
    ret = adc_drv_init(s_channels, NUM_CHANNELS, on_adc_samples);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_drv_init failed: 0x%x", ret);
        voltage_sensor_deinit(s_voltage_sensor);
        current_sensor_deinit(s_current_sensor);
        vTaskDelete(NULL);
        return;
    }

    ret = adc_drv_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_drv_start failed: 0x%x", ret);
        adc_drv_deinit();
        voltage_sensor_deinit(s_voltage_sensor);
        current_sensor_deinit(s_current_sensor);
        vTaskDelete(NULL);
        return;
    }

    // ════════════════════════════════════════════════════════════════════════
    // PHASE 1 — RAW ADC DIAGNOSTICS (5 s)
    // Connect both sensors to live signals before this completes.
    // The output tells you exactly what to put in the defines.
    // ════════════════════════════════════════════════════════════════════════

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== PHASE 1: RAW ADC DIAGNOSTICS (5 s) ===");
    ESP_LOGI(TAG, "Connect ZMPT101B to mains and SCT013 to a live wire now.");
    ESP_LOGI(TAG, "Current config:");
    ESP_LOGI(TAG, "  VOLTAGE — DC offset: %.0f mV | Scale: %.2f",
             (double)VOLTAGE_SENSOR_DC_OFFSET_MV,
             (double)VOLTAGE_SENSOR_SCALE_FACTOR);
    ESP_LOGI(TAG, "  CURRENT — DC offset: %.0f mV | Scale: %.2f",
             (double)CURRENT_SENSOR_DC_OFFSET_MV,
             (double)CURRENT_SENSOR_SCALE_FACTOR);

    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_LOGI(TAG, "--- Voltage sensor (ZMPT101B on CH6/GPIO34) ---");
    print_diagnostics("VOLTAGE",
                      s_v_raw_min, s_v_raw_max,
                      220.0f,                         // change to 230 if your mains is 230V
                      "VOLTAGE_SENSOR_SCALE_FACTOR");

    ESP_LOGI(TAG, "--- Current sensor (SCT013 on CH7/GPIO35) ---");
    print_diagnostics("CURRENT",
                      s_i_raw_min, s_i_raw_max,
                      30.0f,                          // rated full-scale amps
                      "CURRENT_SENSOR_SCALE_FACTOR");

    // ════════════════════════════════════════════════════════════════════════
    // PHASE 2 — DC OFFSET CALIBRATION
    // Both sensors calibrated together:
    //   Voltage sensor: ZMPT101B connected but mains disconnected (primary open)
    //   Current sensor: SCT013 connected but no load on the measured wire
    // 4000 samples = 200ms = 10 full 50Hz cycles.
    // ════════════════════════════════════════════════════════════════════════

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== PHASE 2: DC OFFSET CALIBRATION ===");
    ESP_LOGI(TAG, "Ensure: mains disconnected from ZMPT101B primary,");
    ESP_LOGI(TAG, "        no load on the wire through SCT013.");
    vTaskDelay(pdMS_TO_TICKS(1000));    // give user time to disconnect

    // calibrate_offset() blocks until the ADC task has collected enough
    // samples and the semaphore is given — no polling needed.
    voltage_sensor_calibrate_offset(s_voltage_sensor, 4000);
    current_sensor_calibrate_offset(s_current_sensor, 4000);

    ESP_LOGI(TAG, "Both sensors calibrated — connect mains and load now.");
    vTaskDelay(pdMS_TO_TICKS(2000));

    // ════════════════════════════════════════════════════════════════════════
    // PHASE 3 — WAIT FOR FIRST VALID WINDOW ON BOTH SENSORS
    // ════════════════════════════════════════════════════════════════════════

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== PHASE 3: WAITING FOR VALID WINDOWS ===");

    voltage_sensor_result_t v_result = {0};
    current_sensor_result_t i_result = {0};

    TickType_t timeout = xTaskGetTickCount() + pdMS_TO_TICKS(5000);
    while (xTaskGetTickCount() < timeout) {
        voltage_sensor_get_result(s_voltage_sensor, &v_result);
        current_sensor_get_result(s_current_sensor, &i_result);
        if (v_result.valid && i_result.valid) break;
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (!v_result.valid) ESP_LOGW(TAG, "Voltage sensor window never became valid");
    if (!i_result.valid) ESP_LOGW(TAG, "Current sensor window never became valid");

    // ════════════════════════════════════════════════════════════════════════
    // PHASE 3 — LIVE PRINT LOOP (runs indefinitely)
    // ════════════════════════════════════════════════════════════════════════

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== PHASE 4: LIVE READINGS (1 Hz) ===");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        voltage_sensor_get_result(s_voltage_sensor, &v_result);
        current_sensor_get_result(s_current_sensor, &i_result);

        float v = v_result.valid ? v_result.vrms_v : 0.0f;
        float i = i_result.valid ? i_result.irms_a : 0.0f;
        float s = v * i;   // apparent power (VA) — real power needs phase angle

        ESP_LOGI(TAG, "Vrms: %6.2f V  |  Irms: %5.3f A  |  S: %7.2f VA  |  V:%s I:%s",
                 (double)v,
                 (double)i,
                 (double)s,
                 v_result.valid ? "OK " : "INV",
                 i_result.valid ? "OK " : "INV");
    }
}

// ─── app_main ─────────────────────────────────────────────────────────────────

void app_main(void)
{
    xTaskCreate(test_task, "test_task", 4096, NULL, 5, NULL);
}