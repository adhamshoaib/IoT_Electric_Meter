#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/uart.h"

/**
 * @brief Opaque handle to a UART service instance.
 *        Obtained from uart_service_init, passed to all other functions.
 */
typedef struct uart_service_t *uart_service_handle_t;

/**
 * @brief Configuration for a UART service instance.
 */
typedef struct
{
    uart_port_t port;      // UART peripheral to use (UART_NUM_0, UART_NUM_1, etc.)
    int tx_pin;            // GPIO pin for TX
    int rx_pin;            // GPIO pin for RX
    int baud_rate;         // Baud rate (e.g. 115200)
    size_t rx_buffer_size; // RX ring buffer size in bytes (minimum 128)
    size_t tx_buffer_size; // TX ring buffer size in bytes (0 = blocking TX)
} uart_service_config_t;

/**
 * @brief Initialize the UART service.
 *
 * @param config     Pointer to a populated configuration struct. config baudrate.
 * @param out_handle Output handle set on success, NULL on failure.
 * @return ESP_OK on success.
 *         ESP_ERR_INVALID_ARG if config or out_handle is NULL, or config fields are invalid.
 *         ESP_ERR_NO_MEM if handle allocation fails.
 */
esp_err_t uart_service_init(const uart_service_config_t *config, uart_service_handle_t *out_handle);

/**
 * @brief Send data over UART.
 *
 * @param handle Valid handle from uart_service_init.
 * @param data   Pointer to data buffer to send.
 * @param len    Number of bytes to send (must be > 0).
 * @return ESP_OK on success.
 *         ESP_ERR_INVALID_ARG if handle or data is NULL, or len is 0.
 *         ESP_ERR_INVALID_SIZE if fewer bytes were written than requested.
 *         ESP_FAIL on driver error.
 */
esp_err_t uart_service_send(uart_service_handle_t handle, const uint8_t *data, size_t len);

/**
 * @brief Read data from UART.
 *
 * Blocks until data is available or timeout elapses.
 * A timeout of 0 returns immediately with whatever is in the buffer.
 * Returns ESP_OK with *out_len == 0 if no data arrived within the timeout.
 *
 * @param handle     Valid handle from uart_service_init.
 * @param buf        Buffer to read into.
 * @param max_len    Maximum number of bytes to read (must be > 0).
 * @param out_len    Actual number of bytes read. Always set to 0 on entry.
 * @param timeout_ms Timeout in milliseconds (0 = non-blocking).
 * @return ESP_OK on success (including timeout with no data).
 *         ESP_ERR_INVALID_ARG if handle, buf, or out_len is NULL, or max_len is 0.
 *         ESP_FAIL on driver error.
 */
esp_err_t uart_service_read(uart_service_handle_t handle, uint8_t *buf, size_t max_len, size_t *out_len, uint32_t timeout_ms);

/**
 * @brief Deinitialize the UART service and free all resources.
 *
 * @param handle Pointer to a valid handle. Set to NULL on return.
 * @return ESP_OK on success.
 *         ESP_ERR_INVALID_ARG if handle or *handle is NULL.
 *         ESP_FAIL if the underlying driver could not be deleted.
 */
esp_err_t uart_service_deinit(uart_service_handle_t *handle);