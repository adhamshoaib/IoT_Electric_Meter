#include "BL0939.h"
#include "energy_metering.h"
#include "uart_service.h"
#include "oled_display.h"
#include "i2c_service.h"
#include "wifi_sta.h"
#include "led_driver.h"

#include <stdio.h>
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define OLED_I2C_SDA GPIO_NUM_21
#define OLED_I2C_SCL GPIO_NUM_22
#define OLED_I2C_CLK_HZ 100000
#define OLED_I2C_PORT I2C_NUM_0

#define BL0939_UART_PORT UART_NUM_2
#define BL0939_UART_TX_PIN GPIO_NUM_17
#define BL0939_UART_RX_PIN GPIO_NUM_16
#define BL0939_UART_BAUD_RATE 4800
#define BL0939_DEVICE_ADDRESS 0U

#define BL0939_DEFAULT_TIMEOUT_MS 1500U
#define BL0939_READ_PERIOD_MS 1000U
#define LOAD_CHECK_PERIOD_MS 50U

#define LOAD_LED_GPIO GPIO_NUM_18

static const char *TAG = "MAIN";

static esp_err_t init_meter(uart_service_handle_t *out_uart)
{
    if (out_uart == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const uart_service_config_t uart_cfg = {
        .port = BL0939_UART_PORT,
        .tx_pin = BL0939_UART_TX_PIN,
        .rx_pin = BL0939_UART_RX_PIN,
        .baud_rate = BL0939_UART_BAUD_RATE,
        .rx_buffer_size = 256,
        .tx_buffer_size = 0,
    };

    esp_err_t ret = uart_service_init(&uart_cfg, out_uart);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = uart_service_set_stop_bits(*out_uart, UART_STOP_BITS_2);
    if (ret != ESP_OK)
    {
        (void)uart_service_deinit(out_uart);
        return ret;
    }

    const bl0939_config_t bl_cfg = {
        .uart = *out_uart,
        .device_address = BL0939_DEVICE_ADDRESS,
        .calibration = {
            .voltage_ref = 1.0f,
            .current_ref = 1.0f,
            .energy_ref = 1.0f,
        },
        .phase_compensation = {
            .corner_a = 0x0000,
            .corner_b = 0x0000,
        },
        .current_channel = BL0939_CURRENT_CHANNEL_SUM,
        .default_timeout_ms = BL0939_DEFAULT_TIMEOUT_MS,
        .auto_request_before_read = true,
    };

    ret = bl0939_init(&bl_cfg);
    if (ret != ESP_OK)
    {
        (void)uart_service_deinit(out_uart);
        return ret;
    }

    return ESP_OK;
}

void app_main(void)
{
    uart_service_handle_t uart = NULL;

    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_ret);

    wifi_init();
    ESP_LOGI(TAG, "WiFi connecting in background...");

    esp_err_t ret = init_meter(&uart);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Meter initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    const energy_metering_config_t em_cfg = ENERGY_METERING_CONFIG_DEFAULT();

    ret = energy_metering_init(&em_cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Energy metering initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    const energy_metering_task_config_t em_task_cfg = {
        .task_name = "energy_meter_task",
        .stack_size = 4096U,
        .priority = 5U,
        .period_ms = BL0939_READ_PERIOD_MS,
    };

    ret = energy_metering_start_task(&em_task_cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Energy metering task start failed: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "BL0939 meter started (UART%d TX=%d RX=%d ADDR=%u)",
             BL0939_UART_PORT, BL0939_UART_TX_PIN, BL0939_UART_RX_PIN, (unsigned)BL0939_DEVICE_ADDRESS);

    ret = i2c_service_init(OLED_I2C_PORT, OLED_I2C_SDA, OLED_I2C_SCL, OLED_I2C_CLK_HZ);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C service init failed: %s", esp_err_to_name(ret));
    }

    bool oled_ok = false;
    oled_display_handle_t oled;
    if (ret == ESP_OK)
    {
        ret = oled_display_init(&oled);
        if (ret == ESP_OK)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            oled_display_clear(&oled);
            oled_ok = true;
            ESP_LOGI(TAG, "OLED display initialized");
        }
        else
        {
            ESP_LOGE(TAG, "OLED init failed: %s", esp_err_to_name(ret));
        }
    }

    wifi_wait_connected();
    ESP_LOGI(TAG, "WiFi connected");

    led_t load_led = {.pin = LOAD_LED_GPIO};
    ESP_ERROR_CHECK(led_init(&load_led));

    uint32_t display_tick = 0;

    while (true)
    {
        if (energy_metering_is_load_connected())
        {
            if (load_led.mode != LED_MODE_ON)
                led_on(&load_led);
        }
        else
        {
            if (load_led.mode != LED_MODE_OFF)
                led_off(&load_led);
        }

        if (display_tick == 0U)
        {
            energy_metering_data_t m;
            ret = energy_metering_get_latest(&m);
            if (ret == ESP_OK)
            {
                ESP_LOGI(TAG,
                         "Voltage: %.2f V | Current: %.3f A | Energy: %.6f kWh",
                         m.voltage_v,
                         m.current_a,
                         m.total_energy_kwh);

                if (oled_ok)
                {
                    char line[48];
                    oled_display_clear_buffer(&oled);
                    snprintf(line, sizeof(line), "%.2f", m.total_energy_kwh);
                    int len = strlen(line);
                    int x_start = (len * 12 < 128) ? (128 - len * 12) / 2 : 0;
                    oled_display_write_scaled_string(&oled, x_start, 4, 2, line);
                    oled_display_write_scaled_string(&oled, (128 - 3 * 12) / 2, 36, 2, "kWh");
                    oled_display_flush(&oled);
                }
            }
            else
            {
                ESP_LOGW(TAG, "energy_metering_get_latest failed: %s", esp_err_to_name(ret));
            }
        }

        display_tick = (display_tick + 1U) % 20U;
        vTaskDelay(pdMS_TO_TICKS(LOAD_CHECK_PERIOD_MS));
    }
}
