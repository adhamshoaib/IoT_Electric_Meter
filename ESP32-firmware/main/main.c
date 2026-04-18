#include <stddef.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "ads1115.h"

#define I2C_MASTER_PORT I2C_NUM_0
#define I2C_MASTER_SDA_IO GPIO_NUM_19
#define I2C_MASTER_SCL_IO GPIO_NUM_18
#define I2C_MASTER_FREQ_HZ 10000
#define I2C_PROBE_TIMEOUT_MS 50
#define INIT_RETRY_DELAY_MS 1000

#define ADS1115_A0_CHANNEL 0
#define SAMPLE_PERIOD_MS 500

static const char *TAG = "ADS1115_TEST";

static i2c_master_bus_handle_t s_bus_handle = NULL;
static ads1115_t s_ads = {0};
static uint16_t s_ads_addr = ADS_I2C_ADDR_GND;

static void ads1115_reset_i2c(void)
{
    if (s_ads.i2c_handle != NULL)
    {
        i2c_master_bus_rm_device(s_ads.i2c_handle);
        s_ads.i2c_handle = NULL;
    }

    if (s_bus_handle != NULL)
    {
        i2c_del_master_bus(s_bus_handle);
        s_bus_handle = NULL;
    }
}

static esp_err_t ads1115_probe_address(uint16_t *detected_addr)
{
    const uint16_t candidate_addrs[] = {
        ADS_I2C_ADDR_GND,
        ADS_I2C_ADDR_VDD,
        ADS_I2C_ADDR_SDA,
        ADS_I2C_ADDR_SCL,
    };

    for (size_t i = 0; i < (sizeof(candidate_addrs) / sizeof(candidate_addrs[0])); ++i)
    {
        esp_err_t ret = i2c_master_probe(s_bus_handle, candidate_addrs[i], I2C_PROBE_TIMEOUT_MS);
        if (ret == ESP_OK)
        {
            *detected_addr = candidate_addrs[i];
            return ESP_OK;
        }
    }

    return ESP_ERR_NOT_FOUND;
}

static esp_err_t ads1115_test_init(void)
{
    esp_err_t ret;

    if (s_bus_handle == NULL)
    {
        i2c_master_bus_config_t i2c_mst_config = {
            .i2c_port = I2C_MASTER_PORT,
            .sda_io_num = I2C_MASTER_SDA_IO,
            .scl_io_num = I2C_MASTER_SCL_IO,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };

        gpio_set_pull_mode(I2C_MASTER_SDA_IO, GPIO_PULLUP_ONLY);
        gpio_set_pull_mode(I2C_MASTER_SCL_IO, GPIO_PULLUP_ONLY);

        ret = i2c_new_master_bus(&i2c_mst_config, &s_bus_handle);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "i2c_new_master_bus failed: %s (0x%x)", esp_err_to_name(ret), ret);
            return ret;
        }
    }

    if (s_ads.i2c_handle == NULL)
    {
        ret = ads1115_probe_address(&s_ads_addr);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "No ADS1115 ACK on 0x48..0x4B (internal pull-up enabled)");
            ads1115_reset_i2c();
            return ret;
        }

        ret = ads1115_init(&s_ads, &s_bus_handle, s_ads_addr, I2C_MASTER_FREQ_HZ);

        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "ads1115_init failed: %s (0x%x)", esp_err_to_name(ret), ret);
            ESP_LOGE(TAG, "Check wiring: SDA=GPIO19, SCL=GPIO18, VCC/GND, ADDR strap");
            ads1115_reset_i2c();
            return ret;
        }

        ads1115_set_gain(&s_ads, ADS_FSR_4_096V);
        ads1115_set_sps(&s_ads, ADS_SPS_128);

        ESP_LOGI(TAG, "ADS1115 initialized at 0x%02X (SCL=GPIO18, SDA=GPIO19, internal PU)", s_ads_addr);
        ESP_LOGI(TAG, "Reading voltage sensor from A0 (AIN0) every %d ms", SAMPLE_PERIOD_MS);
    }

    return ESP_OK;
}

void app_main(void)
{
    while (ads1115_test_init() != ESP_OK)
    {
        ESP_LOGW(TAG, "ADS1115 init retry in %d ms", INIT_RETRY_DELAY_MS);
        vTaskDelay(pdMS_TO_TICKS(INIT_RETRY_DELAY_MS));
    }

    while (1)
    {
        uint16_t raw_u16 = ads1115_get_raw(&s_ads, ADS1115_A0_CHANNEL);
        int16_t raw = (int16_t)raw_u16;
        float voltage = ads1115_raw_to_voltage(&s_ads, raw);

        ESP_LOGI(TAG, "A0 raw=%d, voltage=%.4f V", raw, voltage);
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}
