#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "adc_cont.h"

#define TAG "MAIN"

// ADC1_CH6 = GPIO34, ADC1_CH7 = GPIO35 on ESP32
static adc_channel_t s_channels[] = {
    ADC_CHANNEL_6,
    ADC_CHANNEL_7,
};
#define NUM_CHANNELS  (sizeof(s_channels) / sizeof(s_channels[0]))

// ─── shared sample storage (written by callback, read by test_task) ──────────

#define NUM_STORED_CHANNELS 2

static volatile float   s_latest_mv[NUM_STORED_CHANNELS] = {0};
static volatile bool    s_sample_ready = false;

// ─── callback ─────────────────────────────────────────────────────────────────
//
// Runs inside adc_drv_task at high priority — MUST be fast.
// No printf, no float formatting, no blocking calls here.
// Just store the latest value per channel and set a flag.

static void on_adc_samples(const adc_drv_sample_t *samples, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        int ch = (int)samples[i].channel;
        if (ch >= 0 && ch < NUM_STORED_CHANNELS) {
            s_latest_mv[ch] = samples[i].voltage_mv;
        }
    }
    s_sample_ready = true;
}

// ─── test task ────────────────────────────────────────────────────────────────
//
// All blocking work lives here, NOT in app_main.
// app_main runs inside main_task which must not block indefinitely —
// doing so starves the idle task and triggers the WDT.

static void test_task(void *arg)
{
    esp_err_t ret;

    ret = adc_drv_init(s_channels, NUM_CHANNELS, on_adc_samples);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_drv_init failed: 0x%x", ret);
        vTaskDelete(NULL);
        return;
    }

    ret = adc_drv_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_drv_start failed: 0x%x", ret);
        adc_drv_deinit();
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "ADC running -- CH6 (GPIO34) and CH7 (GPIO35)");
    ESP_LOGI(TAG, "Will stop after 10 s, restart for 5 s, then deinit");

    // Print loop — runs in test_task context, safe for float printf.
    // Polls the values written by the callback at a sane rate.
    TickType_t stop_at = xTaskGetTickCount() + pdMS_TO_TICKS(10000);
    while (xTaskGetTickCount() < stop_at) {
        if (s_sample_ready) {
            s_sample_ready = false;
            ESP_LOGI(TAG, "CH6: %.1f mV  |  CH7: %.1f mV",
                     (float)s_latest_mv[ADC_CHANNEL_6],
                     (float)s_latest_mv[ADC_CHANNEL_7]);
        }
        vTaskDelay(pdMS_TO_TICKS(100));   // print at ~10 Hz, not 20 kHz
    }

    ESP_LOGI(TAG, "--- stopping ---");
    ret = adc_drv_stop();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_drv_stop failed: 0x%x", ret);
    }

    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "--- restarting ---");
    ret = adc_drv_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_drv_start failed on restart: 0x%x", ret);
        adc_drv_deinit();
        vTaskDelete(NULL);
        return;
    }

    stop_at = xTaskGetTickCount() + pdMS_TO_TICKS(5000);
    while (xTaskGetTickCount() < stop_at) {
        if (s_sample_ready) {
            s_sample_ready = false;
            ESP_LOGI(TAG, "CH6: %.1f mV  |  CH7: %.1f mV",
                     (float)s_latest_mv[ADC_CHANNEL_6],
                     (float)s_latest_mv[ADC_CHANNEL_7]);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGI(TAG, "--- deinit ---");
    ret = adc_drv_deinit();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_drv_deinit failed: 0x%x", ret);
        vTaskDelete(NULL);
        return;
    }

    // double-deinit guard -- must return ESP_ERR_INVALID_STATE
    ret = adc_drv_deinit();
    ESP_LOGI(TAG, "Double-deinit: 0x%x (expected ESP_ERR_INVALID_STATE = 0x%x)",
             ret, ESP_ERR_INVALID_STATE);

    ESP_LOGI(TAG, "Test complete");
    vTaskDelete(NULL);
}

// ─── app_main ─────────────────────────────────────────────────────────────────
// Must return quickly -- spawns the test task and exits.

void app_main(void)
{
    xTaskCreate(test_task, "test_task", 4096, NULL, 5, NULL);
}