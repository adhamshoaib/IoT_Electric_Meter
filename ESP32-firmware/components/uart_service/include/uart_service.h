#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/uart.h"

typedef struct uart_service_t *uart_service_handle_t;

typedef struct
{
    uart_port_t port; // UART_NUM_1, UART_NUM_2, etc.
    int tx_pin;
    int rx_pin;
    int baud_rate;
    size_t rx_buffer_size; // internal ESP-IDF ring buffer size
    size_t tx_buffer_size; // 0 = no TX buffer (blocking TX)

} uart_service_config_t;

esp_err_t uart_service_init(const uart_service_config_t *config, uart_service_handle_t *out_handle);

esp_err_t uart_service_send(uart_service_handle_t handle, const uint8_t *data, size_t len);

esp_err_t uart_service_read(uart_service_handle_t handle, uint8_t *buf, size_t max_len, size_t *out_len, uint32_t timeout_ms);

esp_err_t uart_service_deinit(uart_service_handle_t *handle);
