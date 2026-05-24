#include "BL0939.h"
#include "energy_metering.h"
#include "uart_service.h"
#include "oled_display.h"

#include <stdio.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define OLED_I2C_SDA GPIO_NUM_21
#define OLED_I2C_SCL GPIO_NUM_22
#define OLED_I2C_CLK_HZ 400000

#define BL0939_UART_PORT UART_NUM_2
#define BL0939_UART_TX_PIN GPIO_NUM_17
#define BL0939_UART_RX_PIN GPIO_NUM_16
#define BL0939_UART_BAUD_RATE 4800
#define BL0939_DEVICE_ADDRESS 0U

#define BL0939_DEFAULT_TIMEOUT_MS 1500U
#define BL0939_READ_PERIOD_MS 1000U

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

    esp_err_t ret = init_meter(&uart);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Meter initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    const energy_metering_config_t em_cfg = {
        .calibration = ENERGY_METERING_CALIBRATION_DEFAULT(),
    };

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

    i2c_master_bus_handle_t i2c_bus = NULL;
    const i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1,
        .scl_io_num = OLED_I2C_SCL,
        .sda_io_num = OLED_I2C_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ret = i2c_new_master_bus(&bus_cfg, &i2c_bus);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C bus init failed: %s", esp_err_to_name(ret));
    }
    bool oled_ok = false;
    oled_display_handle_t oled;
    if (ret == ESP_OK)
    {
        ret = oled_display_init(i2c_bus, &oled);
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

    while (true)
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
                char line[32];
                oled_display_clear_buffer(&oled);
                snprintf(line, sizeof(line), "%.6f", m.total_energy_kwh);
                int len = strlen(line);
                oled_display_write_scaled_string(&oled, (128 - len * 12) / 2, 4, 2, line);
                oled_display_write_scaled_string(&oled, (128 - 3 * 12) / 2, 36, 2, "kWh");
                oled_display_flush(&oled);
            }
        }
        else
        {
            ESP_LOGW(TAG, "energy_metering_get_latest failed: %s", esp_err_to_name(ret));
        }

        vTaskDelay(pdMS_TO_TICKS(BL0939_READ_PERIOD_MS));
    }
}
