#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "adc_wrapper.h"
#include "voltage_sensor.h"
#include "esp_log.h"

#define ADC_VOLTAGE_CHANNEL ADC_CHANNEL_6
#define ADC_CURRENT_CHANNEL ADC_CHANNEL_7

static const char *TAG = "MAIN";

void voltage_rms_test_task(void *arg)
{ 
    // 1. Create ADC wrapper instance
    static adc_wrapper_t adc = {0}; 
    adc.unit = ADC_UNIT_1;

     // 2. Create voltage sensor instance
    voltage_sensor_t sensor = {
        .adc = &adc,
        .channel = ADC_VOLTAGE_CHANNEL,
    };
    // Initialize sensor (just ADC init & channel config)
    if (voltage_sensor_init(&sensor) != ESP_OK) {
        ESP_LOGE(TAG, "Voltage sensor init failed!");
        vTaskDelete(NULL);
    }
    while (1) {
        float rms_mv = 0;

        // Measure RMS voltage
        if (voltage_sensor_read_rms(&sensor, &rms_mv) == ESP_OK) {
            ESP_LOGI(TAG, "RMS Voltage: %.2f V", rms_mv);
        } else {
            ESP_LOGE(TAG, "RMS measurement failed");
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // 1s delay between measurements
    }
}

void app_main(void)
{
    xTaskCreate(voltage_rms_test_task, "voltage_rms_test", 4096, NULL, 5, NULL);
}