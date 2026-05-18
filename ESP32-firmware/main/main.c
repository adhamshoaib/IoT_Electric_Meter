#include "BL0939.h"
#include "uart_service.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BL0939_UART_PORT UART_NUM_2
#define BL0939_UART_TX_PIN GPIO_NUM_17
#define BL0939_UART_RX_PIN GPIO_NUM_16
#define BL0939_UART_BAUD_RATE 4800

#define BL0939_DEFAULT_TIMEOUT_MS 200U
#define BL0939_READ_PERIOD_MS 1000U

static const char *TAG = "MAIN";
static float s_total_energy_kwh = 0.0f;

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

    const bl0939_config_t bl_cfg = {
        .uart = *out_uart,
        .calibration = {
            .voltage_ref = 15200.0f,
            .current_ref = 324004.0f,
            .energy_ref = 3304.0f,
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

static void log_measurements(const bl0939_measurements_t *m)
{
    if (m == NULL)
    {
        return;
    }

    s_total_energy_kwh += m->energy_kwh;
    ESP_LOGI(TAG,
             "Voltage: %.2f V | Current: %.3f A | Energy: %.6f kWh",
             m->voltage_v,
             m->current_a,
             s_total_energy_kwh);
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

    ESP_LOGI(TAG, "BL0939 test started (UART%d TX=%d RX=%d)",
             BL0939_UART_PORT, BL0939_UART_TX_PIN, BL0939_UART_RX_PIN);

    while (true)
    {
        bl0939_measurements_t m;
        ret = bl0939_read_measurements(&m, BL0939_TIMEOUT_USE_DEFAULT);
        if (ret == ESP_OK)
        {
            log_measurements(&m);
        }
        else
        {
            ESP_LOGW(TAG, "bl0939_read_measurements failed: %s", esp_err_to_name(ret));
        }

        vTaskDelay(pdMS_TO_TICKS(BL0939_READ_PERIOD_MS));
    }
}
