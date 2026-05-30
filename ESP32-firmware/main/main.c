#include "BL0939.h"
#include "energy_metering.h"
#include "uart_service.h"
#include "oled_display.h"
#include "i2c_service.h"
#include "wifi_sta.h"
#include "led_driver.h"
#include "cloud_sync.h"
#include "gsm_driver.h"

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

#define BL0939_DEFAULT_TIMEOUT_MS 500U
#define BL0939_READ_PERIOD_MS 1000U
#define LOAD_CHECK_PERIOD_MS 50U

#define LOAD_LED_GPIO GPIO_NUM_18
#define WIFI_LED_GPIO GPIO_NUM_19
#define CLOUD_LED_GPIO GPIO_NUM_23
#define GSM_LED_GPIO GPIO_NUM_27

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
            .voltage_ref = 1.007f,
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

    gsm_err_t gsm_ret = gsm_init();
    if (gsm_ret == GSM_OK)
    {
        ESP_LOGI(TAG, "SIM800 initialised");
        gsm_ret = gsm_wait_for_registration(60000);
        if (gsm_ret != GSM_OK)
            ESP_LOGW(TAG, "GSM registration failed: %s", gsm_err_to_str(gsm_ret));
    }
    else
    {
        ESP_LOGW(TAG, "GSM init failed: %s", gsm_err_to_str(gsm_ret));
    }

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

    if (wifi_is_connected())
    {
        ESP_LOGI(TAG, "WiFi already connected");
    }
    else
    {
        ESP_LOGW(TAG, "WiFi not connected — cloud sync will use GSM fallback");
    }

    ret = cloud_sync_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Cloud sync init failed: %s", esp_err_to_name(ret));
    }
    else
    {
        ret = cloud_sync_start_task();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Cloud sync task start failed: %s", esp_err_to_name(ret));
        }
        else
        {
            ESP_LOGI(TAG, "Cloud sync task started");
        }
    }

    led_t load_led = {.pin = LOAD_LED_GPIO};
    ESP_ERROR_CHECK(led_init(&load_led));

    led_t wifi_led = {.pin = WIFI_LED_GPIO};
    ESP_ERROR_CHECK(led_init(&wifi_led));
    led_off(&wifi_led);

    led_t cloud_led = {.pin = CLOUD_LED_GPIO};
    ESP_ERROR_CHECK(led_init(&cloud_led));
    led_off(&cloud_led);

    led_t gsm_led = {.pin = GSM_LED_GPIO};
    ESP_ERROR_CHECK(led_init(&gsm_led));
    led_off(&gsm_led);

    uint32_t display_tick = 0;
    uint32_t last_upload_count = 0;
    uint32_t elapsed_s = 0;
    TickType_t cloud_blink_until = 0;

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

        if (wifi_is_connected())
        {
            if (wifi_led.mode != LED_MODE_ON)
                led_on(&wifi_led);
        }
        else
        {
            if (wifi_led.mode != LED_MODE_OFF)
                led_off(&wifi_led);
        }

        uint32_t cur_count = cloud_sync_get_upload_count();
        if (cur_count != last_upload_count)
        {
            last_upload_count = cur_count;
            led_on(&cloud_led);
            cloud_blink_until = xTaskGetTickCount() + pdMS_TO_TICKS(100);
        }

        if (cloud_blink_until != 0 && xTaskGetTickCount() >= cloud_blink_until)
        {
            led_off(&cloud_led);
            cloud_blink_until = 0;
        }

        if (cloud_sync_is_gsm_mode())
        {
            if (gsm_led.mode != LED_MODE_ON)
                led_on(&gsm_led);
        }
        else
        {
            if (gsm_led.mode != LED_MODE_OFF)
                led_off(&gsm_led);
        }

        if (display_tick == 0U)
        {
            energy_metering_data_t m;
            elapsed_s += 5U;
            ret = energy_metering_get_latest(&m);
            if (ret == ESP_OK)
            {
                ESP_LOGI(TAG,
                         "Voltage: %.2f V | Current: %.3f A | Energy: %.6f kWh | Elapsed: %u s",
                         m.voltage_v,
                         m.current_a,
                         m.total_energy_kwh,
                         (unsigned)elapsed_s);

                if (oled_ok)
                {
                    char line[48];
                    oled_display_clear_buffer(&oled);
                    snprintf(line, sizeof(line), "%.6f", m.total_energy_kwh);
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

        display_tick = (display_tick + 1U) % 100U;
        vTaskDelay(pdMS_TO_TICKS(LOAD_CHECK_PERIOD_MS));
    }
}
