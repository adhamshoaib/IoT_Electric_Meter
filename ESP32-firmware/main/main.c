#include "ads1115.h"
#include "esp_err.h"
#include "esp_log.h"
#include "current_sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_service.h"
#include "voltage_sensor.h"

#define I2C_PORT I2C_NUM_0
#define I2C_SDA_PIN GPIO_NUM_19
#define I2C_SCL_PIN GPIO_NUM_18
#define I2C_FREQ_HZ 400000

#define ADS1115_VOLTAGE_CHANNEL 0
#define ADS1115_GAIN_VOLTAGE ADS_FSR_4_096V
#define ADS1115_GAIN_CURRENT ADS_FSR_1_024V
#define RMS_SAMPLE_COUNT 128
#define RMS_LOG_PERIOD_MS 1000
#define CURRENT_NOISE_FLOOR_A 0.02f

static const char *TAG = "main";

static ads1115_t s_ads1115;
static int16_t s_voltage_mv_samples[RMS_SAMPLE_COUNT];
static int16_t s_current_mv_samples[RMS_SAMPLE_COUNT];

static float compute_mean_mv(const int16_t *samples, size_t count)
{
    float mean_mv = 0.0f;
    for (size_t i = 0; i < count; ++i)
    {
        mean_mv += (float)samples[i];
    }

    return mean_mv / (float)count;
}

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_service_init(I2C_PORT, I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ));

    i2c_master_bus_handle_t bus_handle = i2c_service_get_bus_handle();
    if (bus_handle == NULL)
    {
        ESP_LOGE(TAG, "Failed to get I2C bus handle");
        return;
    }

    ESP_ERROR_CHECK(ads1115_init(&s_ads1115, &bus_handle, ADS_I2C_ADDR_GND, I2C_FREQ_HZ));
    ads1115_set_gain(&s_ads1115, ADS1115_GAIN_VOLTAGE);
    ads1115_set_sps(&s_ads1115, ADS_SPS_860);
    ESP_LOGI(TAG, "ADS1115 initialized at 0x%02X", ADS_I2C_ADDR_GND);

    while (1)
    {
        for (size_t i = 0; i < RMS_SAMPLE_COUNT; ++i)
        {
            ads1115_set_gain(&s_ads1115, ADS1115_GAIN_VOLTAGE);
            const int16_t raw_voltage = (int16_t)ads1115_get_raw(&s_ads1115, ADS1115_VOLTAGE_CHANNEL);
            const float sensor_voltage_v = ads1115_raw_to_voltage(&s_ads1115, raw_voltage);

            ads1115_set_gain(&s_ads1115, ADS1115_GAIN_CURRENT);
            const int16_t raw_current = ads1115_differential_2_3(&s_ads1115);
            const float sensor_current_v = ads1115_raw_to_voltage(&s_ads1115, raw_current);

            s_voltage_mv_samples[i] = (int16_t)(sensor_voltage_v * 1000.0f);
            s_current_mv_samples[i] = (int16_t)(sensor_current_v * 1000.0f);
        }

        const float voltage_mean_mv = compute_mean_mv(s_voltage_mv_samples, RMS_SAMPLE_COUNT);
        const float current_mean_mv = compute_mean_mv(s_current_mv_samples, RMS_SAMPLE_COUNT);

        const float voltage_rms = voltage_sensor_rms(s_voltage_mv_samples, RMS_SAMPLE_COUNT);
        float current_rms = current_sensor_rms(s_current_mv_samples, RMS_SAMPLE_COUNT);
        if (current_rms < CURRENT_NOISE_FLOOR_A)
        {
            current_rms = 0.0f;
        }
        ESP_LOGI(TAG,
                 "V_rms=%.2f V (mean=%.1f mV) | I_rms=%.3f A (mean=%.1f mV)",
                 voltage_rms,
                 voltage_mean_mv,
                 current_rms,
                 current_mean_mv);

        vTaskDelay(pdMS_TO_TICKS(RMS_LOG_PERIOD_MS));
    }
}
