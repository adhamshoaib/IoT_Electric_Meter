/**
 * @file oled_display.c
 * @brief SSD1306 OLED display driver using i2c_service.
 */

#include "oled_display.h"
#include "i2c_service.h"

#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "oled_display";

static esp_err_t oled_send_cmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};
    return i2c_service_write(OLED_DISPLAY_I2C_ADDRESS, buf, sizeof(buf));
}

static esp_err_t oled_send_data(const uint8_t *data, size_t len)
{
    if (len > 128) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buf[129];
    buf[0] = 0x40;
    memcpy(&buf[1], data, len);
    return i2c_service_write(OLED_DISPLAY_I2C_ADDRESS, buf, len + 1);
}

static const uint8_t s_font_5x7[96][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x5F, 0x00, 0x00},
    {0x00, 0x07, 0x00, 0x07, 0x00},
    {0x14, 0x7F, 0x14, 0x7F, 0x14},
    {0x24, 0x2A, 0x7F, 0x2A, 0x12},
    {0x23, 0x13, 0x08, 0x64, 0x62},
    {0x36, 0x49, 0x55, 0x22, 0x50},
    {0x00, 0x05, 0x03, 0x00, 0x00},
    {0x00, 0x1C, 0x22, 0x41, 0x00},
    {0x00, 0x41, 0x22, 0x1C, 0x00},
    {0x08, 0x2A, 0x1C, 0x2A, 0x08},
    {0x08, 0x08, 0x3E, 0x08, 0x08},
    {0x00, 0x50, 0x30, 0x00, 0x00},
    {0x08, 0x08, 0x08, 0x08, 0x08},
    {0x00, 0x60, 0x60, 0x00, 0x00},
    {0x20, 0x10, 0x08, 0x04, 0x02},
    {0x3E, 0x51, 0x49, 0x45, 0x3E},
    {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46},
    {0x21, 0x41, 0x45, 0x4B, 0x31},
    {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39},
    {0x3C, 0x4A, 0x49, 0x49, 0x30},
    {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36},
    {0x06, 0x49, 0x49, 0x29, 0x1E},
    {0x00, 0x36, 0x36, 0x00, 0x00},
    {0x00, 0x56, 0x36, 0x00, 0x00},
    {0x00, 0x08, 0x14, 0x22, 0x41},
    {0x14, 0x14, 0x14, 0x14, 0x14},
    {0x41, 0x22, 0x14, 0x08, 0x00},
    {0x02, 0x01, 0x51, 0x09, 0x06},
    {0x32, 0x49, 0x79, 0x41, 0x3E},
    {0x7E, 0x11, 0x11, 0x11, 0x7E},
    {0x7F, 0x49, 0x49, 0x49, 0x36},
    {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C},
    {0x7F, 0x49, 0x49, 0x49, 0x41},
    {0x7F, 0x09, 0x09, 0x01, 0x01},
    {0x3E, 0x41, 0x41, 0x51, 0x32},
    {0x7F, 0x08, 0x08, 0x08, 0x7F},
    {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01},
    {0x7F, 0x08, 0x14, 0x22, 0x41},
    {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x04, 0x02, 0x7F},
    {0x7F, 0x04, 0x08, 0x10, 0x7F},
    {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06},
    {0x3E, 0x41, 0x51, 0x21, 0x5E},
    {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31},
    {0x01, 0x01, 0x7F, 0x01, 0x01},
    {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F},
    {0x7F, 0x20, 0x18, 0x20, 0x7F},
    {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x03, 0x04, 0x78, 0x04, 0x03},
    {0x61, 0x51, 0x49, 0x45, 0x43},
    {0x00, 0x00, 0x7F, 0x41, 0x41},
    {0x02, 0x04, 0x08, 0x10, 0x20},
    {0x41, 0x41, 0x7F, 0x00, 0x00},
    {0x04, 0x02, 0x01, 0x02, 0x04},
    {0x40, 0x40, 0x40, 0x40, 0x40},
    {0x00, 0x01, 0x02, 0x04, 0x00},
    {0x20, 0x54, 0x54, 0x54, 0x78},
    {0x7F, 0x48, 0x44, 0x44, 0x38},
    {0x38, 0x44, 0x44, 0x44, 0x20},
    {0x38, 0x54, 0x54, 0x54, 0x18},
    {0x08, 0x7E, 0x09, 0x01, 0x02},
    {0x08, 0x14, 0x54, 0x54, 0x3C},
    {0x7F, 0x08, 0x04, 0x04, 0x78},
    {0x00, 0x44, 0x7D, 0x40, 0x00},
    {0x20, 0x40, 0x44, 0x3D, 0x00},
    {0x00, 0x7F, 0x10, 0x28, 0x44},
    {0x00, 0x41, 0x7F, 0x40, 0x00},
    {0x7C, 0x04, 0x18, 0x04, 0x78},
    {0x7C, 0x08, 0x04, 0x04, 0x78},
    {0x38, 0x44, 0x44, 0x44, 0x38},
    {0x7C, 0x14, 0x14, 0x14, 0x08},
    {0x08, 0x14, 0x14, 0x18, 0x7C},
    {0x7C, 0x08, 0x04, 0x04, 0x08},
    {0x48, 0x54, 0x54, 0x54, 0x20},
    {0x04, 0x3F, 0x44, 0x40, 0x20},
    {0x3C, 0x40, 0x40, 0x20, 0x7C},
    {0x1C, 0x20, 0x40, 0x20, 0x1C},
    {0x3C, 0x40, 0x30, 0x40, 0x3C},
    {0x44, 0x28, 0x10, 0x28, 0x44},
    {0x0C, 0x50, 0x50, 0x50, 0x3C},
    {0x44, 0x64, 0x54, 0x4C, 0x44},
    {0x00, 0x08, 0x36, 0x41, 0x00},
    {0x00, 0x00, 0x7F, 0x00, 0x00},
    {0x00, 0x41, 0x36, 0x08, 0x00},
    {0x08, 0x08, 0x2A, 0x1C, 0x00},
};

esp_err_t oled_display_init(oled_display_handle_t *handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(handle, 0, sizeof(oled_display_handle_t));

    vTaskDelay(pdMS_TO_TICKS(100));

    esp_err_t ret = oled_send_cmd(0xAE);
    if (ret != ESP_OK) return ret;

    ret = oled_send_cmd(0xD5);
    if (ret != ESP_OK) return ret;
    ret = oled_send_cmd(0x80);
    if (ret != ESP_OK) return ret;

    ret = oled_send_cmd(0xA8);
    if (ret != ESP_OK) return ret;
    ret = oled_send_cmd(0x3F);
    if (ret != ESP_OK) return ret;

    ret = oled_send_cmd(0xD3);
    if (ret != ESP_OK) return ret;
    ret = oled_send_cmd(0x00);
    if (ret != ESP_OK) return ret;

    ret = oled_send_cmd(0x40);
    if (ret != ESP_OK) return ret;

    ret = oled_send_cmd(0x8D);
    if (ret != ESP_OK) return ret;
    ret = oled_send_cmd(0x14);
    if (ret != ESP_OK) return ret;
    vTaskDelay(pdMS_TO_TICKS(10));

    ret = oled_send_cmd(0x20);
    if (ret != ESP_OK) return ret;
    ret = oled_send_cmd(0x00);
    if (ret != ESP_OK) return ret;

    ret = oled_send_cmd(0xA0);
    if (ret != ESP_OK) return ret;

    ret = oled_send_cmd(0xC0);
    if (ret != ESP_OK) return ret;

    ret = oled_send_cmd(0xDA);
    if (ret != ESP_OK) return ret;
    ret = oled_send_cmd(0x12);
    if (ret != ESP_OK) return ret;

    ret = oled_send_cmd(0x81);
    if (ret != ESP_OK) return ret;
    ret = oled_send_cmd(0x7F);
    if (ret != ESP_OK) return ret;

    ret = oled_send_cmd(0xD9);
    if (ret != ESP_OK) return ret;
    ret = oled_send_cmd(0xF1);
    if (ret != ESP_OK) return ret;

    ret = oled_send_cmd(0xDB);
    if (ret != ESP_OK) return ret;
    ret = oled_send_cmd(0x40);
    if (ret != ESP_OK) return ret;

    ret = oled_send_cmd(0xA4);
    if (ret != ESP_OK) return ret;

    ret = oled_send_cmd(0xA6);
    if (ret != ESP_OK) return ret;

    vTaskDelay(pdMS_TO_TICKS(100));

    ret = oled_send_cmd(0xAF);
    if (ret != ESP_OK) return ret;

    vTaskDelay(pdMS_TO_TICKS(50));

    handle->cursor_x = 0;
    handle->cursor_y = 0;
    handle->initialized = true;

    handle->frame_buffer = calloc(1, OLED_WIDTH * OLED_HEIGHT / 8);
    if (handle->frame_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate frame buffer");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "OLED display initialized");
    return ESP_OK;
}

esp_err_t oled_display_deinit(oled_display_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    oled_display_turn_off(handle);
    free(handle->frame_buffer);
    memset(handle, 0, sizeof(oled_display_handle_t));
    return ESP_OK;
}

esp_err_t oled_display_clear(oled_display_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t empty_row[OLED_WIDTH] = {0};

    for (uint8_t page = 0; page < 8; page++) {
        esp_err_t ret = oled_send_cmd(0xB0 + page);
        if (ret != ESP_OK) return ret;
        ret = oled_send_cmd(0x00);
        if (ret != ESP_OK) return ret;
        ret = oled_send_cmd(0x10);
        if (ret != ESP_OK) return ret;
        ret = oled_send_data(empty_row, OLED_WIDTH);
        if (ret != ESP_OK) return ret;
    }

    handle->cursor_x = 0;
    handle->cursor_y = 0;
    return ESP_OK;
}

esp_err_t oled_display_turn_on(oled_display_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    return oled_send_cmd(0xAF);
}

esp_err_t oled_display_turn_off(oled_display_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    return oled_send_cmd(0xAE);
}

esp_err_t oled_display_set_cursor(oled_display_handle_t *handle, uint8_t x, uint8_t y)
{
    if (handle == NULL || !handle->initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    if (x >= OLED_WIDTH || y >= 8) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = oled_send_cmd(0xB0 + y);
    if (ret != ESP_OK) return ret;
    ret = oled_send_cmd(0x00 | (x & 0x0F));
    if (ret != ESP_OK) return ret;
    ret = oled_send_cmd(0x10 | ((x >> 4) & 0x0F));
    if (ret == ESP_OK) {
        handle->cursor_x = x;
        handle->cursor_y = y;
    }
    return ret;
}

esp_err_t oled_display_write_string(oled_display_handle_t *handle, const char *str)
{
    if (handle == NULL || str == NULL || !handle->initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    while (*str) {
        uint8_t ch = (uint8_t)*str;
        if (ch >= 0x20 && ch <= 0x7E) {
            if (handle->cursor_x + 6 > OLED_WIDTH) {
                handle->cursor_x = 0;
                handle->cursor_y++;
                if (handle->cursor_y >= 8) {
                    handle->cursor_y = 0;
                }
                esp_err_t ret = oled_display_set_cursor(handle, handle->cursor_x, handle->cursor_y);
                if (ret != ESP_OK) return ret;
            }

            uint8_t char_buffer[6];
            memcpy(char_buffer, s_font_5x7[ch - 0x20], 5);
            char_buffer[5] = 0x00;

            esp_err_t ret = oled_send_data(char_buffer, sizeof(char_buffer));
            if (ret != ESP_OK) return ret;

            handle->cursor_x += 6;
        }
        str++;
    }
    return ESP_OK;
}

esp_err_t oled_display_write_string_at(oled_display_handle_t *handle, uint8_t x, uint8_t y, const char *str)
{
    if (handle == NULL || str == NULL || !handle->initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = oled_display_set_cursor(handle, x, y);
    if (ret != ESP_OK) return ret;
    return oled_display_write_string(handle, str);
}

esp_err_t oled_display_fill_pattern(oled_display_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    for (uint8_t page = 0; page < 8; page++) {
        esp_err_t ret = oled_send_cmd(0xB0 + page);
        if (ret != ESP_OK) return ret;
        ret = oled_send_cmd(0x00);
        if (ret != ESP_OK) return ret;
        ret = oled_send_cmd(0x10);
        if (ret != ESP_OK) return ret;

        uint8_t col_data[16];
        for (int i = 0; i < 16; i++) {
            col_data[i] = (page % 2 == 0) ? 0x55 : 0xAA;
        }
        for (int chunk = 0; chunk < 8; chunk++) {
            ret = oled_send_data(col_data, 16);
            if (ret != ESP_OK) return ret;
        }
    }
    return ESP_OK;
}

esp_err_t oled_display_clear_buffer(oled_display_handle_t *handle)
{
    if (handle == NULL || !handle->initialized || handle->frame_buffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(handle->frame_buffer, 0, OLED_WIDTH * OLED_HEIGHT / 8);
    return ESP_OK;
}

static inline void set_pixel(uint8_t *buffer, uint8_t x, uint8_t y)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    buffer[(y / 8) * OLED_WIDTH + x] |= (1 << (y % 8));
}

esp_err_t oled_display_write_scaled_string(oled_display_handle_t *handle, uint8_t x, uint8_t y, uint8_t scale, const char *str)
{
    if (handle == NULL || str == NULL || !handle->initialized || handle->frame_buffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (scale == 0) scale = 1;

    uint8_t cx = x;
    uint8_t cy = y;
    const uint8_t char_w = 5 * scale + scale;
    const uint8_t char_h = 7 * scale + scale;

    while (*str) {
        uint8_t ch = (uint8_t)(*str);
        if (ch >= 0x20 && ch <= 0x7E) {
            if (cx + char_w > OLED_WIDTH) {
                cx = x;
                cy += char_h;
                if (cy + char_h > OLED_HEIGHT) break;
            }

            const uint8_t *glyph = s_font_5x7[ch - 0x20];
            for (uint8_t fy = 0; fy < 7; fy++) {
                for (uint8_t fx = 0; fx < 5; fx++) {
                    if (glyph[fx] & (1 << fy)) {
                        for (uint8_t sy = 0; sy < scale; sy++) {
                            for (uint8_t sx = 0; sx < scale; sx++) {
                                set_pixel(handle->frame_buffer,
                                          cx + fx * scale + sx,
                                          cy + fy * scale + sy);
                            }
                        }
                    }
                }
            }
            cx += char_w;
        }
        str++;
    }
    return ESP_OK;
}

esp_err_t oled_display_flush(oled_display_handle_t *handle)
{
    if (handle == NULL || !handle->initialized || handle->frame_buffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    for (uint8_t page = 0; page < 8; page++) {
        esp_err_t ret = oled_send_cmd(0xB0 + page);
        if (ret != ESP_OK) return ret;
        ret = oled_send_cmd(0x00);
        if (ret != ESP_OK) return ret;
        ret = oled_send_cmd(0x10);
        if (ret != ESP_OK) return ret;
        ret = oled_send_data(handle->frame_buffer + page * OLED_WIDTH, OLED_WIDTH);
        if (ret != ESP_OK) return ret;
    }
    return ESP_OK;
}
