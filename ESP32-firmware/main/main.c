#include <stdint.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_service.h"

static const char *TAG = "ADS1115_TEST";

#define I2C_PORT_USED I2C_NUM_0
#define I2C_SDA_PIN GPIO_NUM_19
#define I2C_SCL_PIN GPIO_NUM_18
#define I2C_CLK_HZ 400000U

#define ADS1115_ADDR 0x48
#define ADS1115_REG_CONVERSION 0x00
#define ADS1115_REG_CONFIG 0x01

#define ADS1115_CFG_OS_SINGLE (1U << 15)
#define ADS1115_CFG_MUX_AIN0_GND (4U << 12)
#define ADS1115_CFG_PGA_2_048V (2U << 9)
#define ADS1115_CFG_MODE_SINGLE (1U << 8)
#define ADS1115_CFG_DR_860SPS (7U << 5)
#define ADS1115_CFG_COMP_DISABLE (3U)

#define ADS1115_SINGLE_SHOT_CFG (ADS1115_CFG_OS_SINGLE | ADS1115_CFG_MUX_AIN0_GND | ADS1115_CFG_PGA_2_048V | ADS1115_CFG_MODE_SINGLE | ADS1115_CFG_DR_860SPS | ADS1115_CFG_COMP_DISABLE)
#define ADS1115_CONV_READY_TIMEOUT_MS 50U
#define ADS1115_POLL_INTERVAL_MS 2U

static esp_err_t ads1115_start_single_shot(void)
{
    const uint16_t cfg = ADS1115_SINGLE_SHOT_CFG;
    const uint8_t write_buf[3] = {
        ADS1115_REG_CONFIG,
        (uint8_t)(cfg >> 8),
        (uint8_t)(cfg & 0xFF),
    };

    return i2c_service_write(ADS1115_ADDR, write_buf, sizeof(write_buf));
}

static esp_err_t ads1115_wait_conversion_ready(uint32_t timeout_ms)
{
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    if (timeout_ticks == 0)
    {
        timeout_ticks = 1;
    }

    const TickType_t start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < timeout_ticks)
    {
        uint8_t cfg_buf[2] = {0};
        esp_err_t ret = i2c_service_write_read(ADS1115_ADDR, ADS1115_REG_CONFIG, cfg_buf, sizeof(cfg_buf));
        if (ret != ESP_OK)
        {
            return ret;
        }

        const uint16_t cfg = ((uint16_t)cfg_buf[0] << 8) | cfg_buf[1];
        if ((cfg & ADS1115_CFG_OS_SINGLE) != 0)
        {
            return ESP_OK;
        }

        vTaskDelay(pdMS_TO_TICKS(ADS1115_POLL_INTERVAL_MS));
    }

    return ESP_ERR_TIMEOUT;
}

static esp_err_t ads1115_read_single_ain0(int16_t *out_raw)
{
    if (out_raw == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(ads1115_start_single_shot(), TAG, "failed to start conversion");
    ESP_RETURN_ON_ERROR(ads1115_wait_conversion_ready(ADS1115_CONV_READY_TIMEOUT_MS), TAG, "conversion timeout");

    uint8_t raw_buf[2] = {0};
    ESP_RETURN_ON_ERROR(i2c_service_write_read(ADS1115_ADDR, ADS1115_REG_CONVERSION, raw_buf, sizeof(raw_buf)), TAG, "failed to read conversion register");

    *out_raw = (int16_t)(((uint16_t)raw_buf[0] << 8) | raw_buf[1]);
    return ESP_OK;
}

void app_main(void)
{
    esp_err_t ret = i2c_service_init(I2C_PORT_USED, I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLK_HZ);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "i2c_service_init failed: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "I2C init OK (SDA=%d, SCL=%d, addr=0x%02X)", I2C_SDA_PIN, I2C_SCL_PIN, ADS1115_ADDR);
    ESP_LOGI(TAG, "Reading ADS1115 AIN0 every second...");

    while (1)
    {
        int16_t raw = 0;
        ret = ads1115_read_single_ain0(&raw);
        if (ret == ESP_OK)
        {
            const float volts = (float)raw * 0.0000625f;
            ESP_LOGI(TAG, "AIN0 raw=%d, voltage=%.6f V", raw, volts);
        }
        else
        {
            ESP_LOGW(TAG, "ADS1115 read failed: %s", esp_err_to_name(ret));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
