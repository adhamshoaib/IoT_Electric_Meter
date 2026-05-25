#pragma once

/**
 * @file led_driver.h
 * @brief ESP32 LED driver with support for GPIO control and PWM brightness/effects
 *
 * This driver provides a flexible LED control interface with the following features:
 * - Simple on/off control via GPIO
 * - PWM-based brightness control (0-255)
 * - Blink effect with configurable period
 * - Smooth glow (breathing) effect with gamma correction
 *
 * The driver uses a background FreeRTOS task to handle animations and mode changes,
 * allowing non-blocking control from the main application.
 */

#include <stdint.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "stdbool.h"
#include "freertos/queue.h"

/**
 * @enum led_mode_t
 * @brief LED operating mode
 */
typedef enum
{
    LED_MODE_OFF,   ///< LED is off
    LED_MODE_ON,    ///< LED is on (steady, full brightness)
    LED_MODE_BLINK, ///< LED is blinking at specified period
    LED_MODE_GLOW,  ///< LED is in smooth glow (breathing) mode
} led_mode_t;

/**
 * @enum led_cmd_type_t
 * @brief Internal command types for the LED worker task
 */
typedef enum
{
    LED_CMD_SET_MODE,       ///< Change LED mode (on/off/blink/glow)
    LED_CMD_SET_BRIGHTNESS, ///< Set brightness level (0-255)
    LED_CMD_SET_PERIOD,     ///< Change animation period
    LED_CMD_STOP            ///< Stop worker task (internal use)
} led_cmd_type_t;

/**
 * @struct led_cmd_t
 * @brief Command sent to LED worker task
 * @internal
 */
typedef struct
{
    led_cmd_type_t type; ///< Command type
    led_mode_t mode;     ///< Target mode (for SET_MODE command)
    uint8_t brightness;  ///< Brightness level 0-255 (for SET_BRIGHTNESS command)
    uint32_t period_ms;  ///< Period in milliseconds (for SET_MODE/SET_PERIOD)
} led_cmd_t;

/**
 * @struct led_t
 * @brief LED control structure
 *
 * Must be initialized by the caller before use.
 *
 * **Required fields before led_init():**
 * - `pin`: GPIO pin number (must be set)
 *
 * **Required fields before led_pwm_init():**
 * - `channel`: LEDC channel (0 to LEDC_CHANNEL_MAX-1) - MUST be explicitly set
 * - `timer`: LEDC timer (0 to LEDC_TIMER_MAX-1) - MUST be explicitly set
 *
 * Failure to initialize `channel` and `timer` before led_pwm_init() will result
 * in validation errors or unpredictable behavior.
 *
 * @note The last_error field can be read at any time to check for worker task failures.
 *       It is automatically cleared when a new command is sent.
 *
 * @example GPIO-only initialization:
 * ```c
 * led_t led = { .pin = GPIO_NUM_18 };
 * led_init(&led);
 * led_on(&led);
 * ```
 *
 * @example PWM initialization:
 * ```c
 * led_t led = {
 *     .pin = GPIO_NUM_18,
 *     .channel = LEDC_CHANNEL_0,
 *     .timer = LEDC_TIMER_0
 * };
 * led_init(&led);
 * led_pwm_init(&led);
 * led_set_brightness(&led, 128);
 * ```
 */
typedef struct
{
    // ========== Hardware Configuration ==========
    /// GPIO pin number (REQUIRED - must be set before led_init())
    gpio_num_t pin;

    /// LEDC channel for PWM (REQUIRED for PWM - must be set before led_pwm_init())
    /// Valid range: 0 to LEDC_CHANNEL_MAX-1
    /// Must be unique if multiple PWM LEDs are used
    ledc_channel_t channel;

    /// LEDC timer for PWM (REQUIRED for PWM - must be set before led_pwm_init())
    /// Valid range: 0 to LEDC_TIMER_MAX-1
    /// Can be shared among multiple channels
    ledc_timer_t timer;

    /// PWM initialized flag (read-only, set by led_pwm_init)
    bool pwm_initialized;

    // ========== Logical State ==========
    /// Current LED mode (read-only, reflects actual LED state)
    led_mode_t mode;

    /// Current animation period in milliseconds (read-only)
    uint32_t period_ms;

    // ========== Runtime State ==========
    /// Toggle state for blink mode (internal)
    bool toggle_on;

    /// Worker task handle (internal)
    TaskHandle_t worker_task;

    /// Command queue handle (internal)
    QueueHandle_t cmd_q;

    /// Last error from worker task (read-only, cleared on new commands)
    esp_err_t last_error;
} led_t;

//============================================================================//
// Basic GPIO Control
//============================================================================//

/**
 * @brief Initialize LED control on specified GPIO pin
 *
 * Sets up GPIO configuration and starts the background worker task.
 * Must be called before any other led_* function.
 *
 * For GPIO-only control, this is sufficient. For PWM effects, also call led_pwm_init().
 *
 * @param[in,out] led LED control structure with pin field initialized
 *
 * @return
 *   - ESP_OK: LED initialized successfully
 *   - ESP_ERR_INVALID_ARG: Invalid LED or pin number
 *   - ESP_ERR_NO_MEM: Failed to create command queue
 *   - ESP_FAIL: Failed to create worker task
 *
 * @note The pin field in led_t must be set before calling this function
 *
 * @example
 * ```c
 * led_t led = { .pin = GPIO_NUM_18 };
 * esp_err_t ret = led_init(&led);
 * if (ret != ESP_OK) {
 *     ESP_LOGE(TAG, "LED init failed: %s", esp_err_to_name(ret));
 * }
 * ```
 */
esp_err_t led_init(led_t *led);

/**
 * @brief Deinitialize LED and clean up resources
 *
 * Stops the worker task, closes the command queue, and turns off the LED.
 * After calling this, the LED object is no longer usable.
 *
 * @param[in,out] led LED control structure
 *
 * @return
 *   - ESP_OK: LED deinitialized successfully
 *   - ESP_ERR_INVALID_ARG: NULL pointer passed
 *   - Other: Error codes from underlying hardware drivers
 *
 * @example
 * ```c
 * led_deinit(&led);
 * ```
 */
esp_err_t led_deinit(led_t *led);

/**
 * @brief Turn LED on to full brightness
 *
 * Sets the LED to steady on state. For GPIO mode, this sets the pin to HIGH.
 * For PWM mode, this sets duty to maximum (100%).
 *
 * Non-blocking; command is sent to worker task.
 *
 * @param[in,out] led LED control structure
 *
 * @return
 *   - ESP_OK: Command sent successfully
 *   - ESP_ERR_INVALID_ARG: NULL pointer passed
 *   - ESP_ERR_INVALID_STATE: LED not initialized
 *   - ESP_FAIL: Failed to send command to worker
 *
 * @example
 * ```c
 * led_on(&led);
 * ```
 */
esp_err_t led_on(led_t *led);

/**
 * @brief Turn LED off
 *
 * Sets the LED to off state. For GPIO mode, this sets the pin to LOW.
 * For PWM mode, this sets duty to 0%.
 *
 * Non-blocking; command is sent to worker task.
 *
 * @param[in,out] led LED control structure
 *
 * @return
 *   - ESP_OK: Command sent successfully
 *   - ESP_ERR_INVALID_ARG: NULL pointer passed
 *   - ESP_ERR_INVALID_STATE: LED not initialized
 *   - ESP_FAIL: Failed to send command to worker
 *
 * @example
 * ```c
 * led_off(&led);
 * ```
 */
esp_err_t led_off(led_t *led);

/**
 * @brief Start LED blinking at specified period
 *
 * Toggles the LED on and off at the specified period. For GPIO mode, this
 * digitally toggles the pin. For PWM mode, this toggles between 0% and 100% duty.
 *
 * The blink period is the total time for one complete on-off cycle.
 * E.g., 500ms means 250ms on + 250ms off.
 *
 * Non-blocking; command is sent to worker task.
 *
 * @param[in,out] led LED control structure
 * @param[in] period Blink period in milliseconds (must be > 0)
 *
 * @return
 *   - ESP_OK: Command sent successfully
 *   - ESP_ERR_INVALID_ARG: NULL pointer or period == 0
 *   - ESP_ERR_INVALID_STATE: LED not initialized
 *   - ESP_FAIL: Failed to send command to worker
 *
 * @note Minimum practical period is limited by vTaskDelay granularity (typically 10ms)
 *
 * @example
 * ```c
 * led_blink(&led, 500);  // Blink every 500ms (250ms on, 250ms off)
 * vTaskDelay(pdMS_TO_TICKS(5000));
 * if (led.last_error != ESP_OK) {
 *     ESP_LOGE(TAG, "Blink failed: %s", esp_err_to_name(led.last_error));
 * }
 * ```
 */
esp_err_t led_blink(led_t *led, uint32_t period);

//============================================================================//
// PWM Control (Requires led_pwm_init)
//============================================================================//

/**
 * @brief Initialize PWM mode for the LED
 *
 * Sets up LEDC (LED PWM Controller) for brightness control and effects.
 * Must be called after led_init() if PWM functionality is desired.
 *
 * Configures:
 * - Frequency: 5 kHz (inaudible, smooth PWM)
 * - Resolution: 12-bit (0-4095 duty cycle)
 * - Speed mode: Low speed mode
 *
 * @param[in,out] led LED control structure with channel and timer fields initialized
 *
 * @return
 *   - ESP_OK: PWM initialized successfully
 *   - ESP_ERR_INVALID_ARG: Invalid LED, pin, channel, or timer
 *   - ESP_ERR_INVALID_STATE: PWM already initialized
 *   - Other: Error codes from LEDC driver
 *
 * @note The channel and timer fields in led_t must be set and unique
 *       (not shared with other LEDC peripherals) before calling this function
 *
 * @example
 * ```c
 * led_t led = {
 *     .pin = GPIO_NUM_18,
 *     .channel = LEDC_CHANNEL_0,
 *     .timer = LEDC_TIMER_0
 * };
 * led_init(&led);
 * esp_err_t ret = led_pwm_init(&led);
 * if (ret != ESP_OK) {
 *     ESP_LOGE(TAG, "PWM init failed: %s", esp_err_to_name(ret));
 * }
 * ```
 */
esp_err_t led_pwm_init(led_t *led);

/**
 * @brief Set LED brightness level
 *
 * Adjusts LED brightness via PWM duty cycle. Requires PWM mode to be initialized.
 * Setting brightness to 0 is equivalent to turning the LED off.
 *
 * Non-blocking; command is sent to worker task.
 *
 * @param[in,out] led LED control structure
 * @param[in] brightness Brightness level from 0 (off) to 255 (full brightness)
 *
 * @return
 *   - ESP_OK: Command sent successfully
 *   - ESP_ERR_INVALID_ARG: NULL pointer passed
 *   - ESP_ERR_INVALID_STATE: LED not initialized or PWM not initialized
 *   - ESP_FAIL: Failed to send command to worker
 *
 * @note Values > 255 are not clamped; behavior is undefined
 *
 * @example
 * ```c
 * led_pwm_init(&led);
 * led_set_brightness(&led, 128);   // 50% brightness
 * led_set_brightness(&led, 255);   // Full brightness
 * led_set_brightness(&led, 0);     // Off
 * ```
 */
esp_err_t led_set_brightness(led_t *led, uint8_t brightness);

/**
 * @brief Start smooth glow (breathing) effect
 *
 * Creates a smooth inhale-exhale animation using cosine wave modulation with
 * gamma correction for perceptually smooth brightness transitions.
 *
 * The glow period is the time for one complete full cycle (dim → bright → dim).
 *
 * Requires PWM mode to be initialized.
 * Non-blocking; command is sent to worker task.
 *
 * @param[in,out] led LED control structure
 * @param[in] period Full glow cycle period in milliseconds
 *
 * @return
 *   - ESP_OK: Command sent successfully
 *   - ESP_ERR_INVALID_ARG: NULL pointer passed
 *   - ESP_ERR_INVALID_STATE: LED not initialized or PWM not initialized
 *   - ESP_FAIL: Failed to send command to worker
 *
 * @note Typical period: 1000-3000ms for natural breathing effect
 * @note The glow animation is computed with high precision; CPU usage increases
 *       slightly compared to simple on/off or blinking
 *
 * @example
 * ```c
 * led_pwm_init(&led);
 * led_glow(&led, 2000);  // 2-second breathing cycle
 * vTaskDelay(pdMS_TO_TICKS(10000));
 * if (led.last_error != ESP_OK) {
 *     ESP_LOGE(TAG, "Glow failed: %s", esp_err_to_name(led.last_error));
 * }
 * ```
 */
esp_err_t led_glow(led_t *led, uint32_t period);