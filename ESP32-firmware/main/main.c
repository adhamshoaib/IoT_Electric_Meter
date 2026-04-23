#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "uart_service.h"

static const char *TAG = "main";

static uart_service_handle_t uart_handle;

#define BL_FULL_PACKET 0xAA
#define BL_MAX_FRAME_SIZE 35

#define BL0939_VREF 1.218f
#define BL0939_VRMS_SCALE 79931.0f
#define VP_AT_220V 0.0844f
#define MAINS_CALIB_VOLT 220.0f
#define DIVIDER_RATIO (MAINS_CALIB_VOLT / VP_AT_220V)

#define BL0939_SEL_PIN (-1)
#define BL0939_SEL_ACTIVE_LEVEL 1

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

#ifndef UART_SIGNAL_INV_DISABLE
#define UART_SIGNAL_INV_DISABLE 0
#endif

typedef struct
{
    uint32_t baud_rate;
    uint8_t read_cmd;
    size_t frame_size;
    uart_stop_bits_t stop_bits;
    uint32_t inverse_mask;
} bl_mode_t;

typedef struct
{
    uint8_t reg;
    uint8_t data0;
    uint8_t data1;
    uint8_t data2;
} bl_init_cmd_t;

static uart_service_config_t uart_config = {
    .port = UART_NUM_2,
    .tx_pin = GPIO_NUM_17,
    .rx_pin = GPIO_NUM_16,
    .baud_rate = 4800,
    .rx_buffer_size = 256,
    .tx_buffer_size = 0,
};

static const bl_init_cmd_t bl_init_cmds[] = {
    {0x19, 0x5A, 0x5A, 0x5A},
    {0x1A, 0x55, 0x00, 0x00},
    {0x18, 0x00, 0x10, 0x00},
    {0x1B, 0xFF, 0x47, 0x00},
    {0x10, 0x1C, 0x18, 0x00},
    {0x1E, 0x1C, 0x18, 0x00},
};

static uint32_t decode_u24(const uint8_t *p)
{
    return ((uint32_t)p[2] << 16) | ((uint32_t)p[1] << 8) | p[0];
}

static int32_t decode_s24(const uint8_t *p)
{
    int32_t value = ((int32_t)p[2] << 16) | ((int32_t)p[1] << 8) | p[0];
    if ((value & 0x00800000) != 0)
        value |= (int32_t)0xFF000000;
    return value;
}

static uint16_t decode_u16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[1] << 8) | p[0]);
}

static float vrms_raw_to_vp_mv(uint32_t v_rms_raw)
{
    float vp_rms_v = ((float)v_rms_raw * BL0939_VREF) / BL0939_VRMS_SCALE;
    return vp_rms_v * 1000.0f;
}

static float vrms_raw_to_mains_v(uint32_t v_rms_raw)
{
    float vp_rms_v = ((float)v_rms_raw * BL0939_VREF) / BL0939_VRMS_SCALE;
    return vp_rms_v * DIVIDER_RATIO;
}

static const char *inverse_name(uint32_t inverse_mask)
{
    if (inverse_mask == UART_SIGNAL_INV_DISABLE)
        return "none";
    if (inverse_mask == UART_SIGNAL_RXD_INV)
        return "RX";
    if (inverse_mask == UART_SIGNAL_TXD_INV)
        return "TX";
    if (inverse_mask == (UART_SIGNAL_RXD_INV | UART_SIGNAL_TXD_INV))
        return "RX+TX";
    return "custom";
}

static const char *stop_bits_name(uart_stop_bits_t stop_bits)
{
    if (stop_bits == UART_STOP_BITS_1)
        return "1";
    if (stop_bits == UART_STOP_BITS_1_5)
        return "1.5";
    if (stop_bits == UART_STOP_BITS_2)
        return "2";
    return "?";
}

static uint8_t write_cmd_from_read(uint8_t read_cmd)
{
    return (uint8_t)(0xA0 | (read_cmd & 0x0F));
}

static uint8_t bl_checksum(const uint8_t *frame, size_t frame_size, uint8_t read_cmd)
{
    uint8_t checksum = read_cmd;

    for (size_t i = 0; i < frame_size - 1; i++)
        checksum += frame[i];

    checksum ^= 0xFF;
    return checksum;
}

static bool is_valid_header(uint8_t b)
{
    return (b == 0x55) || (b == 0x58);
}

static void configure_sel_pin(void)
{
#if BL0939_SEL_PIN >= 0
    gpio_config_t sel_cfg = {
        .pin_bit_mask = 1ULL << BL0939_SEL_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&sel_cfg);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "SEL pin config failed: %s", esp_err_to_name(err));
        return;
    }

    err = gpio_set_level(BL0939_SEL_PIN, BL0939_SEL_ACTIVE_LEVEL);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "SEL pin set level failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "SEL pin GPIO%d set to %d", BL0939_SEL_PIN, BL0939_SEL_ACTIVE_LEVEL);
#else
    ESP_LOGI(TAG, "SEL pin is not controlled in firmware (BL0939_SEL_PIN = -1)");
#endif
}

static esp_err_t apply_uart_mode(const bl_mode_t *mode)
{
    esp_err_t err = uart_set_baudrate(uart_config.port, mode->baud_rate);
    if (err != ESP_OK)
        return err;

    err = uart_set_stop_bits(uart_config.port, mode->stop_bits);
    if (err != ESP_OK)
        return err;

    err = uart_set_line_inverse(uart_config.port, (uart_signal_inv_t)mode->inverse_mask);
    if (err != ESP_OK)
        return err;

    return ESP_OK;
}

static esp_err_t send_write_command(uint8_t write_cmd, const bl_init_cmd_t *cmd)
{
    uint8_t frame[6] = {write_cmd, cmd->reg, cmd->data0, cmd->data1, cmd->data2, 0};
    uint8_t crc = 0;

    for (size_t i = 0; i < 5; i++)
        crc += frame[i];

    frame[5] = (uint8_t)(crc ^ 0xFF);
    return uart_service_send(uart_handle, frame, sizeof(frame));
}

static esp_err_t run_init_sequence(uint8_t write_cmd)
{
    for (size_t i = 0; i < ARRAY_LEN(bl_init_cmds); i++)
    {
        esp_err_t err = send_write_command(write_cmd, &bl_init_cmds[i]);
        if (err != ESP_OK)
            return err;
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    return ESP_OK;
}

static esp_err_t request_frame(uint8_t read_cmd)
{
    const uint8_t cmd[2] = {read_cmd, BL_FULL_PACKET};
    return uart_service_send(uart_handle, cmd, sizeof(cmd));
}

static esp_err_t read_frame(uint8_t *frame, size_t frame_size, uint32_t timeout_ms)
{
    uint8_t chunk[64];
    size_t frame_index = 0;
    TickType_t start = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

    while ((xTaskGetTickCount() - start) < timeout_ticks)
    {
        size_t bytes_read = 0;
        esp_err_t ret = uart_service_read(uart_handle, chunk, sizeof(chunk), &bytes_read, 40);
        if (ret != ESP_OK)
            return ret;

        if (bytes_read == 0)
            continue;

        for (size_t i = 0; i < bytes_read; i++)
        {
            uint8_t b = chunk[i];

            if (frame_index == 0)
            {
                if (is_valid_header(b))
                    frame[frame_index++] = b;
                continue;
            }

            frame[frame_index++] = b;
            if (frame_index == frame_size)
                return ESP_OK;
        }
    }

    return ESP_ERR_TIMEOUT;
}

static void log_frame_values(const uint8_t *frame, size_t frame_size)
{
    if (frame_size == 35)
    {
        uint32_t ia_rms = decode_u24(&frame[4]);
        uint32_t ib_rms = decode_u24(&frame[7]);
        uint32_t v_rms_raw = decode_u24(&frame[10]);
        float vp_mv = vrms_raw_to_vp_mv(v_rms_raw);
        float mains_v = vrms_raw_to_mains_v(v_rms_raw);
        int32_t a_watt = decode_s24(&frame[16]);
        int32_t b_watt = decode_s24(&frame[19]);

        ESP_LOGI(TAG,
                 "Decoded 35B: VrmsRaw=%lu Vp=%.3f mV Vac=%.2f IaRms=%lu IbRms=%lu A_W=%ld B_W=%ld",
                 (unsigned long)v_rms_raw,
                 vp_mv,
                 mains_v,
                 (unsigned long)ia_rms,
                 (unsigned long)ib_rms,
                 (long)a_watt,
                 (long)b_watt);
        return;
    }

    if (frame_size == 23)
    {
        uint32_t ia_rms = decode_u24(&frame[1]);
        uint32_t v_rms = decode_u24(&frame[4]);
        int32_t a_watt = decode_s24(&frame[10]);
        uint16_t freq = decode_u16(&frame[16]);

        ESP_LOGI(TAG,
                 "Decoded 23B: Vrms=%lu IaRms=%lu A_W=%ld FreqRaw=%u",
                 (unsigned long)v_rms,
                 (unsigned long)ia_rms,
                 (long)a_watt,
                 (unsigned int)freq);
    }
}

static bool try_mode(const bl_mode_t *mode, uint8_t *frame_out)
{
    esp_err_t err = apply_uart_mode(mode);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "Mode setup failed (baud=%lu stop=%s inv=%s): %s",
                 (unsigned long)mode->baud_rate,
                 stop_bits_name(mode->stop_bits),
                 inverse_name(mode->inverse_mask),
                 esp_err_to_name(err));
        return false;
    }

    uint8_t write_cmd = write_cmd_from_read(mode->read_cmd);
    err = run_init_sequence(write_cmd);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Init sequence TX failed (cmd=0x%02X): %s", write_cmd, esp_err_to_name(err));
        return false;
    }

    for (int attempt = 0; attempt < 3; attempt++)
    {
        uart_flush_input(uart_config.port);

        err = request_frame(mode->read_cmd);
        if (err != ESP_OK)
            return false;

        err = read_frame(frame_out, mode->frame_size, 500);
        if (err == ESP_ERR_TIMEOUT)
            continue;
        if (err != ESP_OK)
            return false;

        uint8_t expected = bl_checksum(frame_out, mode->frame_size, mode->read_cmd);
        uint8_t received = frame_out[mode->frame_size - 1];
        if (expected != received)
            continue;

        ESP_LOGI(TAG,
                 "Detected mode: baud=%lu read=0x%02X frame=%u stop=%s inv=%s",
                 (unsigned long)mode->baud_rate,
                 mode->read_cmd,
                 (unsigned)mode->frame_size,
                 stop_bits_name(mode->stop_bits),
                 inverse_name(mode->inverse_mask));
        ESP_LOG_BUFFER_HEX(TAG, frame_out, mode->frame_size);
        log_frame_values(frame_out, mode->frame_size);
        return true;
    }

    return false;
}

void app_main(void)
{
    esp_err_t err = uart_service_init(&uart_config, &uart_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_service_init failed: %s", esp_err_to_name(err));
        return;
    }

    configure_sel_pin();

    uint8_t frame[BL_MAX_FRAME_SIZE] = {0};
    bl_mode_t active_mode = {
        .baud_rate = 4800,
        .read_cmd = 0x50,
        .frame_size = 35,
        .stop_bits = UART_STOP_BITS_1_5,
        .inverse_mask = UART_SIGNAL_INV_DISABLE,
    };

    ESP_LOGI(TAG,
             "Using fixed mode: baud=%lu read=0x%02X frame=%u stop=%s inv=%s",
             (unsigned long)active_mode.baud_rate,
             active_mode.read_cmd,
             (unsigned)active_mode.frame_size,
             stop_bits_name(active_mode.stop_bits),
             inverse_name(active_mode.inverse_mask));

    err = apply_uart_mode(&active_mode);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to apply fixed mode: %s", esp_err_to_name(err));
        return;
    }

    uint8_t write_cmd = write_cmd_from_read(active_mode.read_cmd);
    err = run_init_sequence(write_cmd);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Init sequence TX failed (cmd=0x%02X): %s", write_cmd, esp_err_to_name(err));
        return;
    }

    int consecutive_failures = 0;

    while (1)
    {
        bool frame_ok = false;

        for (int attempt = 0; attempt < 2; attempt++)
        {
            err = request_frame(active_mode.read_cmd);
            if (err != ESP_OK)
            {
                if (attempt == 1)
                    ESP_LOGW(TAG, "Request TX failed: %s", esp_err_to_name(err));
                continue;
            }

            err = read_frame(frame, active_mode.frame_size, 1000);
            if (err == ESP_ERR_TIMEOUT)
            {
                if (attempt == 1)
                    ESP_LOGW(TAG, "No response from BL09xx after retry. Check SEL/address/pull-up.");
                continue;
            }
            if (err != ESP_OK)
            {
                if (attempt == 1)
                    ESP_LOGW(TAG, "Read error: %s", esp_err_to_name(err));
                continue;
            }

            uint8_t expected = bl_checksum(frame, active_mode.frame_size, active_mode.read_cmd);
            uint8_t received = frame[active_mode.frame_size - 1];
            if (expected != received)
            {
                if (attempt == 1)
                {
                    ESP_LOGW(TAG, "Checksum mismatch: expected 0x%02X got 0x%02X", expected, received);
                    ESP_LOG_BUFFER_HEX(TAG, frame, active_mode.frame_size);
                }
                continue;
            }

            frame_ok = true;
            break;
        }

        if (frame_ok)
        {
            consecutive_failures = 0;
            ESP_LOGI(TAG, "Frame OK (%u bytes)", (unsigned)active_mode.frame_size);
            log_frame_values(frame, active_mode.frame_size);
        }
        else
        {
            consecutive_failures++;
        }

        if (consecutive_failures >= 5)
        {
            ESP_LOGW(TAG, "Too many consecutive failures (%d). Retrying fixed BL09xx mode.", consecutive_failures);

            if (try_mode(&active_mode, frame))
            {
                ESP_LOGI(TAG,
                         "Recovery success: baud=%lu read=0x%02X frame=%u stop=%s inv=%s",
                         (unsigned long)active_mode.baud_rate,
                         active_mode.read_cmd,
                         (unsigned)active_mode.frame_size,
                         stop_bits_name(active_mode.stop_bits),
                         inverse_name(active_mode.inverse_mask));
                consecutive_failures = 0;
            }
            else
            {
                ESP_LOGW(TAG, "Recovery retry failed on fixed BL09xx mode.");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
