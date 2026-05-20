#include "BL0939.h"
#include "energy_metering.h"
#include "uart_service.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BL0939_UART_PORT UART_NUM_2
#define BL0939_UART_TX_PIN GPIO_NUM_17
#define BL0939_UART_RX_PIN GPIO_NUM_16
#define BL0939_UART_BAUD_RATE 4800
#define BL0939_DEVICE_ADDRESS 0U /* A4..A1 strap: 0..15 (e.g. 5 -> read cmd 0x55) */

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
        /* energy_metering owns engineering-unit calibration; raw reads still require >0 divisors */
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
        }
        else
        {
            ESP_LOGW(TAG, "energy_metering_get_latest failed: %s", esp_err_to_name(ret));
        }

        vTaskDelay(pdMS_TO_TICKS(BL0939_READ_PERIOD_MS));
    }
}
