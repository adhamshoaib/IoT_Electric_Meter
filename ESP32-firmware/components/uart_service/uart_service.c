#include "uart_service.h"

#include <stdlib.h>
#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

// Minimum RX ring buffer size enforced by ESP-IDF
#define UART_SERVICE_MIN_RX_BUF_SIZE 128

static const char *TAG = "UART_SERVICE";

// Internal handle — fields hidden from callers via opaque pointer in header
struct uart_service_t
{
    uart_port_t port;
};

esp_err_t uart_service_init(const uart_service_config_t *config, uart_service_handle_t *out_handle)
{
    if (!config || !out_handle || config->port < 0 || config->port >= SOC_UART_NUM || config->rx_buffer_size < UART_SERVICE_MIN_RX_BUF_SIZE)
        return ESP_ERR_INVALID_ARG;

    *out_handle = NULL;

    struct uart_service_t *handle = calloc(1, sizeof(struct uart_service_t));
    if (!handle)
        return ESP_ERR_NO_MEM;

    handle->port = config->port;

    uart_config_t uart_config = {
        .baud_rate = config->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Order matters: param_config before driver_install
    esp_err_t ret = uart_param_config(config->port, &uart_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(ret));
        goto cleanup;
    }

    ret = uart_driver_install(config->port, config->rx_buffer_size, config->tx_buffer_size, 0, NULL, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(ret));
        goto cleanup;
    }

    ret = uart_set_pin(config->port, config->tx_pin, config->rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(ret));
        goto cleanup_driver; // driver installed, must be deleted before returning
    }

    ESP_LOGI(TAG, "UART%d initialized at %d baud", handle->port, config->baud_rate);
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

esp_err_t uart_service_send(uart_service_handle_t handle, const uint8_t *data, size_t len)
{
    if (!handle || !data || len == 0)
        return ESP_ERR_INVALID_ARG;

    // uart_write_bytes returns bytes written or -1 on error
    int err = uart_write_bytes(handle->port, data, len);
    if (err == -1)
        return ESP_FAIL;
    if ((size_t)err < len)
    {
        ESP_LOGW(TAG, "Partial write on UART%d: %d of %d bytes sent", handle->port, err, (int)len);
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGD(TAG, "TX %d bytes on UART%d", (int)len, handle->port);
    return ESP_OK;
}

esp_err_t uart_service_read(uart_service_handle_t handle, uint8_t *buf, size_t max_len, size_t *out_len, uint32_t timeout_ms)
{
    if (!handle || !buf || max_len == 0 || !out_len)
        return ESP_ERR_INVALID_ARG;

    *out_len = 0;

    // uart_read_bytes returns bytes read or -1 on error
    // 0 bytes is not an error — it means timeout elapsed with no data
    int err = uart_read_bytes(handle->port, buf, (uint32_t)max_len, pdMS_TO_TICKS(timeout_ms));
    if (err == -1)
        return ESP_FAIL;

    *out_len = (size_t)err;
    ESP_LOGD(TAG, "RX %d bytes on UART%d", err, handle->port);
    return ESP_OK;
}

esp_err_t uart_service_deinit(uart_service_handle_t *handle)
{
    if (!handle || !*handle)
        return ESP_ERR_INVALID_ARG;

    // Log port before freeing — handle is invalid after free
    ESP_LOGI(TAG, "UART%d deinitializing", (*handle)->port);

    esp_err_t err = uart_driver_delete((*handle)->port);
    if (err != ESP_OK)
        ESP_LOGE(TAG, "uart_driver_delete failed: %s", esp_err_to_name(err));

    free(*handle);
    *handle = NULL;

    return err;
}