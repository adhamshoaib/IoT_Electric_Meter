#include "voltage_sensor.h"
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"  
#include "esp32/rom/ets_sys.h"    

esp_err_t voltage_sensor_init(voltage_sensor_t *sensor)
{
    if (!sensor || !sensor->adc) {
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize ADC wrapper if not already initialized
    if (!sensor->adc->initialized) {
        esp_err_t ret = adc_wrapper_init(sensor->adc, sensor->adc->unit);
        if (ret != ESP_OK) {
            ESP_LOGE("VOLT_SENSOR", "ADC wrapper init failed: %d", ret);
            return ret;
        }
    }

    // Configure the channel if not already done
    if (!sensor->adc->configured[sensor->channel]) {
        esp_err_t ret = adc_wrapper_config_channel(sensor->adc, sensor->channel, VOLTAGE_ATTEN_DB);
        if (ret != ESP_OK) {
            ESP_LOGE("VOLT_SENSOR", "ADC channel config failed: %d", ret);
            return ret;
        }
    }

    return ESP_OK;
}


esp_err_t voltage_sensor_read_rms(voltage_sensor_t *sensor, float *out_rms)
{
    if (!sensor || !sensor->adc || !out_rms) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!sensor->adc->initialized || !sensor->adc->configured[sensor->channel]) {
        return ESP_ERR_INVALID_STATE;
    }

    int64_t sum_sq = 0;
    int reading;

    for (int i = 0; i < RMS_TOTAL_SAMPLES; i++) {
        esp_err_t ret = adc_wrapper_read_mv(sensor->adc, sensor->channel, &reading);
        if (ret != ESP_OK) {
            return ret;
        }

        reading -= VOLTAGE_DC_OFFSET;            // remove DC offset
        sum_sq += (int64_t)reading * reading;       // accumulate squared samples

        // Small delay between samples for proper sampling rate
        ets_delay_us((uint32_t)SAMPLE_DELAY_US);
    }

    float mean_sq = (float)sum_sq / RMS_TOTAL_SAMPLES;
    float rms_volts = sqrtf(mean_sq) / 1000.0f * VOLTAGE_SCALE_FACTOR;

    *out_rms = rms_volts;
    return ESP_OK;
}

