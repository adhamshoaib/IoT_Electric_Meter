#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "uart_service.h"

// Connect GPIO4 (TX) to GPIO5 (RX) with a jumper wire for this test
#define TEST_TX_PIN 17
#define TEST_RX_PIN 16
#define TEST_BAUD_RATE 115200
#define TEST_UART_PORT UART_NUM_2
#define TEST_BUF_SIZE 256
#define TEST_TIMEOUT_MS 1000

static const char *TAG = "MAIN";

void app_main(void)
{
    // --- Init ---
    uart_service_handle_t handle = NULL;

    uart_service_config_t config = {
        .port = TEST_UART_PORT,
        .tx_pin = TEST_TX_PIN,
        .rx_pin = TEST_RX_PIN,
        .baud_rate = TEST_BAUD_RATE,
        .rx_buffer_size = TEST_BUF_SIZE,
        .tx_buffer_size = 0, // blocking TX
    };

    esp_err_t ret = uart_service_init(&config, &handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Init failed: %s", esp_err_to_name(ret));
        return;
    }

    // --- Send ---
    const char *msg = "hello uart";
    ret = uart_service_send(handle, (const uint8_t *)msg, strlen(msg));
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Send failed: %s", esp_err_to_name(ret));
        goto done;
    }

    // Small delay to let TX complete before reading back
    vTaskDelay(pdMS_TO_TICKS(100));

    // --- Read ---
    uint8_t rx_buf[TEST_BUF_SIZE];
    size_t rx_len = 0;

    ret = uart_service_read(handle, rx_buf, sizeof(rx_buf), &rx_len, TEST_TIMEOUT_MS);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Read failed: %s", esp_err_to_name(ret));
        goto done;
    }

    // --- Verify ---
    if (rx_len == 0)
    {
        ESP_LOGW(TAG, "No data received — check TX/RX are bridged");
        goto done;
    }

    if (rx_len == strlen(msg) && memcmp(rx_buf, msg, rx_len) == 0)
        ESP_LOGI(TAG, "PASS: received \"%.*s\"", (int)rx_len, rx_buf);
    else
        ESP_LOGE(TAG, "FAIL: received %d bytes, expected %d", (int)rx_len, (int)strlen(msg));

done:
    uart_service_deinit(&handle);
}