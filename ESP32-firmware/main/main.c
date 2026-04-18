#include "ads1115.h"
#include "esp_err.h"
#include "esp_log.h"
#include "i2c_service.h"

#define I2C_PORT I2C_NUM_0
#define I2C_SDA_PIN GPIO_NUM_19
#define I2C_SCL_PIN GPIO_NUM_18
#define I2C_FREQ_HZ 400000

static const char *TAG = "main";

static ads1115_t s_ads1115;

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
    ESP_LOGI(TAG, "ADS1115 initialized at 0x%02X", ADS_I2C_ADDR_GND);
}
