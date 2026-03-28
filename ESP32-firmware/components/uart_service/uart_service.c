#include "uart_service.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_check.h"

static const char *TAG = "UART_SERVICE";

struct uart_service_t
{
    uart_port_t port; // needed by send, read, deinit
};

esp_err_t uart_service_init(const uart_service_config_t *config, uart_service_handle_t *out_handle)
{
    // 1. Validate inputs
    if (!config || !out_handle)
        return ESP_ERR_INVALID_ARG;

    // 2. Allocate handle (calloc zeros all fields)
    struct uart_service_t *handle = calloc(1, sizeof(struct uart_service_t));
    if (!handle)
        return ESP_ERR_NO_MEM;

    // 3. Store what we need for later
    handle->port = config->port;

    // 4. Configure UART
    uart_config_t uart_config = {
        .baud_rate = config->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret = uart_param_config(config->port, &uart_config);
    if (ret != ESP_OK)
        goto cleanup;

    ret = uart_driver_install(config->port, config->rx_buffer_size, config->tx_buffer_size, 0, NULL, 0);
    if (ret != ESP_OK)
        goto cleanup;

    ret = uart_set_pin(config->port, config->tx_pin, config->rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK)
        goto cleanup_driver;

    *out_handle = handle;
    return ESP_OK;

cleanup:
    free(handle);
    return ret;

cleanup_driver:
    uart_driver_delete(config->port);
    free(handle);
    return ret;
}
