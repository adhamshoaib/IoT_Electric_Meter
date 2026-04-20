#include "ads1115.h"
#include "esp_err.h"
#include "esp_log.h"
#include "http_client.h"
#include "current_sensor.h"
#include "energy_calc.h"
#include "energy_config.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_service.h"
#include "lwip/apps/sntp.h"
#include "nvs_flash.h"
#include "voltage_sensor.h"
#include "wifi.h"

#include <sys/time.h>
#include <time.h>

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
#define ADS1115_SAMPLE_RATE_SPS 860.0f
#define WINDOW_DURATION_S ((float)RMS_SAMPLE_COUNT / ADS1115_SAMPLE_RATE_SPS)
#define CLOUD_POST_PERIOD_MS 5000

static const char *TAG = "main";

static ads1115_t s_ads1115;
static adc_sample_pair_t s_adc_samples[RMS_SAMPLE_COUNT];
static int16_t s_current_raw_samples[RMS_SAMPLE_COUNT];
static float s_voltage_samples_v[RMS_SAMPLE_COUNT];
static float s_current_samples_a[RMS_SAMPLE_COUNT];
static float s_energy_kwh = 0.0f;

static void obtain_time(void)
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    time_t now = 0;
    struct tm timeinfo = {0};
    while (timeinfo.tm_year < (2020 - 1900))
    {
        time(&now);
        localtime_r(&now, &timeinfo);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "Time synchronized");
}

static float compute_mean_raw(const int16_t *samples, size_t count)
{
    float mean_raw = 0.0f;
    for (size_t i = 0; i < count; ++i)
    {
        mean_raw += (float)samples[i];
    }

    return mean_raw / (float)count;
}

static float compute_voltage_mean_raw(const adc_sample_pair_t *pairs, size_t count)
{
    float mean_raw = 0.0f;
    for (size_t i = 0; i < count; ++i)
    {
        mean_raw += (float)pairs[i].v_raw;
    }

    return mean_raw / (float)count;
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    wifi_init();
    wifi_wait_connected();
    ESP_LOGI(TAG, "WiFi connected, starting SNTP");
    obtain_time();

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

    TickType_t last_cloud_post_tick = xTaskGetTickCount();

    while (1)
    {
        for (size_t i = 0; i < RMS_SAMPLE_COUNT; ++i)
        {
            ads1115_set_gain(&s_ads1115, ADS1115_GAIN_VOLTAGE);
            const int16_t raw_voltage = (int16_t)ads1115_get_raw(&s_ads1115, ADS1115_VOLTAGE_CHANNEL);

            ads1115_set_gain(&s_ads1115, ADS1115_GAIN_CURRENT);
            const int16_t raw_current = ads1115_differential_2_3(&s_ads1115);

            s_adc_samples[i].v_raw = raw_voltage;
            s_adc_samples[i].i_raw = raw_current;
            s_current_raw_samples[i] = raw_current;

            s_voltage_samples_v[i] = voltage_sensor_convert(raw_voltage);
            s_current_samples_a[i] = current_sensor_convert(raw_current);
        }

        const float voltage_mean_raw = compute_voltage_mean_raw(s_adc_samples, RMS_SAMPLE_COUNT);
        const float current_mean_raw = compute_mean_raw(s_current_raw_samples, RMS_SAMPLE_COUNT);

        const float voltage_rms = voltage_sensor_rms(s_adc_samples, RMS_SAMPLE_COUNT);
        float current_rms = current_sensor_rms(s_current_raw_samples, RMS_SAMPLE_COUNT);
        if (current_rms < CURRENT_NOISE_FLOOR_A)
        {
            current_rms = 0.0f;
        }

        power_calc_result_t power = {0};
        ESP_ERROR_CHECK(energy_calc_power(&power,
                                          s_voltage_samples_v,
                                          s_current_samples_a,
                                          RMS_SAMPLE_COUNT,
                                          CURRENT_PHASE_ADVANCE_SAMPLES,
                                          VOLTAGE_PHASE_ADVANCE_SAMPLES));
        ESP_ERROR_CHECK(energy_calc_accumulate_kwh(&s_energy_kwh,
                                                   power.real_power_w,
                                                   WINDOW_DURATION_S));

        const TickType_t now_tick = xTaskGetTickCount();
        if ((now_tick - last_cloud_post_tick) >= pdMS_TO_TICKS(CLOUD_POST_PERIOD_MS))
        {
            last_cloud_post_tick = now_tick;

            time_t ts = 0;
            time(&ts);

            if (wifi_is_connected())
            {
                const esp_err_t post_err = firebase_post(ts, s_energy_kwh);
                if (post_err != ESP_OK)
                {
                    ESP_LOGW(TAG, "Cloud post failed: %s", esp_err_to_name(post_err));
                }
            }
            else
            {
                ESP_LOGW(TAG, "WiFi disconnected, skip cloud post");
            }
        }

        ESP_LOGI(TAG,
                 "V_rms=%.2f V (mean=%.1f raw) | I_rms=%.3f A (mean=%.1f raw) | P=%.2f W | S=%.2f VA | PF=%.3f | E=%.6f kWh",
                 voltage_rms,
                 voltage_mean_raw,
                 current_rms,
                 current_mean_raw,
                 power.real_power_w,
                 power.apparent_power_va,
                 power.power_factor,
                 s_energy_kwh);

        vTaskDelay(pdMS_TO_TICKS(RMS_LOG_PERIOD_MS));
    }
}
