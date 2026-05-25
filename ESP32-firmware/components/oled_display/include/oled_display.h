/**
 * @file oled_display.h
 * @brief SSD1306 OLED display driver using i2c_service.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define OLED_DISPLAY_I2C_ADDRESS 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

typedef struct {
    uint8_t cursor_x;
    uint8_t cursor_y;
    bool initialized;
    uint8_t *frame_buffer;
} oled_display_handle_t;

esp_err_t oled_display_init(oled_display_handle_t *handle);
esp_err_t oled_display_deinit(oled_display_handle_t *handle);
esp_err_t oled_display_clear(oled_display_handle_t *handle);
esp_err_t oled_display_turn_on(oled_display_handle_t *handle);
esp_err_t oled_display_turn_off(oled_display_handle_t *handle);
esp_err_t oled_display_set_cursor(oled_display_handle_t *handle, uint8_t x, uint8_t y);
esp_err_t oled_display_write_string(oled_display_handle_t *handle, const char *str);
esp_err_t oled_display_write_string_at(oled_display_handle_t *handle, uint8_t x, uint8_t y, const char *str);
esp_err_t oled_display_fill_pattern(oled_display_handle_t *handle);

esp_err_t oled_display_clear_buffer(oled_display_handle_t *handle);
esp_err_t oled_display_write_scaled_string(oled_display_handle_t *handle, uint8_t x, uint8_t y, uint8_t scale, const char *str);
esp_err_t oled_display_flush(oled_display_handle_t *handle);
