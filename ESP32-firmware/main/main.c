#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "adc_cont.h"
#include "voltage_sensor.h"

#define TAG "MAIN"

#define VOLTAGE_SENSOR_CHANNEL   ADC_CHANNEL_6

static adc_channel_t s_channels[] = { VOLTAGE_SENSOR_CHANNEL };
#define NUM_CHANNELS (sizeof(s_channels) / sizeof(s_channels[0]))

static voltage_sensor_ctx_t *s_voltage_sensor = NULL;

// ─── ADC callback — fast path, no printf ─────────────────────────────────────

static void on_adc_samples(const adc_drv_sample_t *samples, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        if (samples[i].channel == VOLTAGE_SENSOR_CHANNEL) {
            voltage_sensor_feed(s_voltage_sensor, samples[i].voltage_mv);
        }
    }
}

// ─── test task ────────────────────────────────────────────────────────────────

static void test_task(void *arg)
{
    esp_err_t ret;

    ret = voltage_sensor_init(VOLTAGE_SENSOR_CHANNEL, &s_voltage_sensor);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "voltage_sensor_init failed: 0x%x", ret);
        vTaskDelete(NULL);
        return;
    }

    ret = adc_drv_init(s_channels, NUM_CHANNELS, on_adc_samples);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_drv_init failed: 0x%x", ret);
        voltage_sensor_deinit(s_voltage_sensor);
        vTaskDelete(NULL);
        return;
    }

    ret = adc_drv_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_drv_start failed: 0x%x", ret);
        adc_drv_deinit();
        voltage_sensor_deinit(s_voltage_sensor);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Voltage sensor running — CH6 (GPIO34)");
    ESP_LOGI(TAG, "DC offset: %.0f mV | Scale: %.2f | Window: %d samples (%.1f ms)",
             (double)VOLTAGE_SENSOR_DC_OFFSET_MV,
             (double)VOLTAGE_SENSOR_SCALE_FACTOR,
             VOLTAGE_SENSOR_WINDOW_SIZE,
             (double)(VOLTAGE_SENSOR_WINDOW_SIZE * 1000.0f / 20000.0f));

    // Wait for first window to fill
    voltage_sensor_result_t result;
    do {
        vTaskDelay(pdMS_TO_TICKS(50));
        voltage_sensor_get_result(s_voltage_sensor, &result);
    } while (!result.valid);

    ESP_LOGI(TAG, "First window ready — printing Vrms every second");

    // Print Vrms every 1 second
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        voltage_sensor_get_result(s_voltage_sensor, &result);

        if (result.valid) {
            ESP_LOGI(TAG, "Vrms = %.2f V", (double)result.vrms_v);
        } else {
            ESP_LOGW(TAG, "No valid result yet");
        }
    }
}

// ─── app_main ─────────────────────────────────────────────────────────────────

void app_main(void)
{
    xTaskCreate(test_task, "test_task", 4096, NULL, 5, NULL);
}