/**
 * @file led_driver.c
 * @brief ESP32 LED driver implementation
 *
 * This implementation provides flexible LED control using a background FreeRTOS task
 * for non-blocking operation. The driver supports:
 *
 * - GPIO-based digital control (on/off/toggle)
 * - PWM-based brightness control (0-255 levels)
 * - Blink animations with configurable period
 * - Smooth glow (breathing) effect with gamma-corrected brightness
 *
 * ## Architecture
 *
 * The driver is built around a worker task that processes commands from a queue:
 *
 * ```
 * Caller (main app)
 *    |
 *    ├─ led_init() ─────► Create worker task + command queue
 *    |
 *    ├─ led_on() ───────┐
 *    ├─ led_off() ──────┼─► xQueueOverwrite() ─► Worker Task ─► Hardware (GPIO/LEDC)
 *    ├─ led_blink() ────┤
 *    └─ led_glow() ─────┘
 *
 *                        Check for errors:
 *                        if (led.last_error != ESP_OK) ...
 * ```
 *
 * ## Implementation Notes
 *
 * - The worker task does not block the calling thread
 * - Commands are queued; only the latest command is kept (xQueueOverwrite)
 * - Error tracking allows caller to detect hardware failures asynchronously
 * - PWM resolution: 12-bit (0-4095), frequency: 5 kHz
 * - Glow effect: Cosine wave with 2.2 gamma correction for perceptual linearity
 */

#include "led_driver.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "driver/ledc.h"
#include "math.h"
#include "sys/param.h"

static const char *TAG = "LED_DRIVER";

// ============================================================================
// PWM Configuration
// ============================================================================

/// PWM duty resolution: 12-bit gives 0-4095 range
#define LED_PWM_DUTY_RESOLUTION LEDC_TIMER_12_BIT
/// Maximum duty cycle value (2^12 - 1)
#define LED_PWM_MAX_DUTY ((1 << LED_PWM_DUTY_RESOLUTION) - 1)

// ============================================================================
// Animation Period Limits
// ============================================================================

/// Minimum blink/glow period in milliseconds (prevents CPU spin)
#define LED_MIN_PERIOD_MS 50U
/// Maximum blink/glow period in milliseconds (prevents overflow in calculations)
#define LED_MAX_PERIOD_MS 3600000U // 1 hour

// ============================================================================
// Worker Task Configuration
// ============================================================================

/// FreeRTOS task priority for LED worker task (5 = medium priority, suitable for background animation control)
#define LED_WORKER_TASK_PRIORITY 5

// ============================================================================
// Glow Effect Parameters
// ============================================================================

/// Number of steps per half-cycle for glow animation smoothness
#define GLOW_STEPS 50U

/// Gamma correction factor for perceptual brightness (2.2 = sRGB gamma)
#define LED_GLOW_GAMMA 2.2f
/// Pi constant for cosine-based glow waveform
#define LED_PI_F 3.14159265358979323846f

// ============================================================================
// Internal Helper Functions
// ============================================================================

/**
 * @brief Apply "on" state to LED (internal)
 *
 * Sets LED to full brightness. Uses PWM if initialized, GPIO otherwise.
 *
 * @param led LED control structure
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t led_apply_on(led_t *led)
{
    if (!led)
        return ESP_ERR_INVALID_ARG;

    if (led->pwm_initialized)
    {
        ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, led->channel, LED_PWM_MAX_DUTY), TAG, "Failed to set duty to max");
        ESP_RETURN_ON_ERROR(ledc_update_duty(LEDC_LOW_SPEED_MODE, led->channel), TAG, "Failed to update duty");
    }
    else
    {
        ESP_RETURN_ON_ERROR(gpio_set_level(led->pin, 1), TAG, "gpio_set_level failed");
    }

    return ESP_OK;
}

/**
 * @brief Apply "off" state to LED (internal)
 *
 * Sets LED to zero brightness. Uses PWM if initialized, GPIO otherwise.
 *
 * @param led LED control structure
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t led_apply_off(led_t *led)
{
    if (!led)
        return ESP_ERR_INVALID_ARG;

    if (led->pwm_initialized)
    {
        ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, led->channel, 0), TAG, "Failed to set duty to zero");
        ESP_RETURN_ON_ERROR(ledc_update_duty(LEDC_LOW_SPEED_MODE, led->channel), TAG, "Failed to update duty");
    }
    else
    {
        ESP_RETURN_ON_ERROR(gpio_set_level(led->pin, 0), TAG, "gpio_set_level failed");
    }

    return ESP_OK;
}

/**
 * @brief Toggle LED state (internal)
 *
 * Switches LED between on and off. Used by blink mode.
 * Tracks toggle state in led->toggle_on.
 *
 * @param led LED control structure
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t led_apply_toggle(led_t *led)
{
    if (!led)
        return ESP_ERR_INVALID_ARG;

    if (led->pwm_initialized)
    {
        ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, led->channel, led->toggle_on ? 0 : LED_PWM_MAX_DUTY), TAG, "UNABLE TO  SET DUTY TO MAX");
        ESP_RETURN_ON_ERROR(ledc_update_duty(LEDC_LOW_SPEED_MODE, led->channel), TAG, "UNABLE TO UPDATE DUTY TO MAX");
    }
    else
    {
        ESP_RETURN_ON_ERROR(gpio_set_level(led->pin, !led->toggle_on), TAG, "gpio_set_level failed");
    }

    led->toggle_on = !led->toggle_on;

    return ESP_OK;
}

// ============================================================================
// PWM Helper Functions
// ============================================================================

/**
 * @brief Set LED brightness level (internal)
 *
 * Converts 0-255 brightness to PWM duty cycle and applies to LEDC.
 * Requires PWM to be initialized.
 *
 * @param led LED control structure
 * @param brightness 0-255 brightness level
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t led_apply_brightness(led_t *led, uint8_t brightness)
{
    if (!led)
        return ESP_ERR_INVALID_ARG;

    if (!led->pwm_initialized)
        return ESP_ERR_INVALID_STATE;

    uint32_t duty = (brightness * LED_PWM_MAX_DUTY) / 255;

    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, led->channel, duty), TAG, "Failed to set duty");
    ESP_RETURN_ON_ERROR(ledc_update_duty(LEDC_LOW_SPEED_MODE, led->channel), TAG, "Failed to update duty");

    return ESP_OK;
}

/**
 * @brief Advance glow animation by one step (internal)
 *
 * Computes the next brightness value for the glow (breathing) effect.
 * Uses cosine wave for smooth oscillation with gamma correction for
 * perceptually linear brightness changes.
 *
 * Algorithm:
 * 1. Advance phase based on elapsed time (dt_ms / period_ms)
 * 2. Generate smooth envelope using cosine: s = 0.5 * (1 - cos(2π * phase))
 * 3. Apply gamma correction: y = s^2.2 (perceptual brightness)
 * 4. Convert to PWM duty cycle
 *
 * @param led LED control structure
 * @param[in,out] phase Current animation phase (0.0 to 1.0), wraps at boundaries
 * @param dt_ms Elapsed time since last step in milliseconds
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t led_apply_glow_step(led_t *led, float *phase, uint32_t dt_ms)
{
    if (!led || !phase || dt_ms == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Advance phase
    float dt = (float)dt_ms / (float)led->period_ms;
    *phase += dt;
    while (*phase >= 1.0f)
    {
        *phase -= 1.0f;
    }
    while (*phase < 0.0f)
    {
        *phase += 1.0f;
    }

    // Smooth inhale/exhale shape: 0..1..0 (cosine)
    float s = 0.5f * (1.0f - cosf(2.0f * LED_PI_F * (*phase)));

    // Perceptual correction (gamma)
    float y = powf(s, LED_GLOW_GAMMA);

    // Convert to duty
    uint32_t duty = (uint32_t)lroundf(y * (float)LED_PWM_MAX_DUTY);

    if (led->pwm_initialized)
    {
        ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, led->channel, duty), TAG, "set duty failed");
        ESP_RETURN_ON_ERROR(ledc_update_duty(LEDC_LOW_SPEED_MODE, led->channel), TAG, "update duty failed");
    }
    else
    {
        bool on = (duty > 0);
        ESP_RETURN_ON_ERROR(gpio_set_level(led->pin, on ? 1 : 0), TAG, "gpio_set_level failed");
    }

    return ESP_OK;
}

// ============================================================================
// Worker Task
// ============================================================================

/**
 * @brief Background worker task for LED control (internal)
 *
 * This task handles all LED state changes and animations in the background,
 * allowing the caller to send non-blocking commands via the command queue.
 *
 * Task behavior:
 * - LED_MODE_OFF / LED_MODE_ON: No periodic activity, responds only to commands
 * - LED_MODE_BLINK: Toggles LED at led->period_ms intervals
 * - LED_MODE_GLOW: Continuously updates brightness for smooth breathing effect
 *
 * Command Queue:
 * - Receives led_cmd_t structures via xQueueOverwrite (last command wins)
 * - Clears last_error when a new command is processed
 * - Stores any hardware errors in last_error for caller to check
 *
 * @param arg Pointer to led_t structure (passed via xTaskCreate)
 */
static void led_worker(void *arg)
{
    led_t *led = (led_t *)arg;
    led_cmd_t cmd;
    float glow_phase = 0.0f;

    for (;;)
    {
        TickType_t wait_ticks = portMAX_DELAY;

        // Periodic wake only for animated modes
        if (led->mode == LED_MODE_BLINK)
        {
            wait_ticks = MAX((TickType_t)1, pdMS_TO_TICKS(led->period_ms));
        }
        else if (led->mode == LED_MODE_GLOW)
        {
            wait_ticks = MAX((TickType_t)1, pdMS_TO_TICKS(led->period_ms / (2U * GLOW_STEPS)));
        }

        BaseType_t got_cmd = xQueueReceive(led->cmd_q, &cmd, wait_ticks);

        if (got_cmd == pdTRUE)
        {
            esp_err_t err = ESP_OK;

            switch (cmd.type)
            {
            case LED_CMD_SET_MODE:
                led->mode = cmd.mode;
                led->last_error = ESP_OK; // Clear error on new command
                if (cmd.period_ms)
                    led->period_ms = cmd.period_ms;

                if (led->mode == LED_MODE_OFF)
                {
                    err = led_apply_off(led);
                }
                else if (led->mode == LED_MODE_ON)
                {
                    err = led_apply_on(led);
                }
                else if (led->mode == LED_MODE_BLINK)
                {
                    // optional reset behavior
                    led->toggle_on = false;
                }
                else if (led->mode == LED_MODE_GLOW)
                {
                    glow_phase = 0.0f;
                }
                break;

            case LED_CMD_SET_PERIOD:
                led->last_error = ESP_OK; // Clear error on new command
                if (cmd.period_ms > 0)
                {
                    led->period_ms = cmd.period_ms;
                }
                break;

            case LED_CMD_SET_BRIGHTNESS:
                led->last_error = ESP_OK; // Clear error on new command
                err = led_apply_brightness(led, cmd.brightness);
                if (err == ESP_OK)
                {
                    led->mode = (cmd.brightness == 0) ? LED_MODE_OFF : LED_MODE_ON;
                }
                break;

            case LED_CMD_STOP:
                (void)led_apply_off(led);
                vTaskDelete(NULL);
                return;

            default:
                break;
            }

            if (err != ESP_OK)
            {
                led->last_error = err; // ← Store error instead of just logging
                ESP_LOGW(TAG, "worker cmd failed: %s", esp_err_to_name(err));
            }

            continue; // command handled
        }

        // Timeout path = periodic mode tick
        if (led->mode == LED_MODE_BLINK)
        {
            esp_err_t err = led_apply_toggle(led);
            if (err != ESP_OK)
            {
                led->last_error = err; // ← Store blink errors
                ESP_LOGW(TAG, "blink step failed: %s", esp_err_to_name(err));
            }
        }
        else if (led->mode == LED_MODE_GLOW)
        {
            uint32_t step_ms = MAX(1U, led->period_ms / (2U * GLOW_STEPS));
            esp_err_t err = led_apply_glow_step(led, &glow_phase, step_ms);
            if (err != ESP_OK)
            {
                led->last_error = err; // ← Store glow errors
                ESP_LOGW(TAG, "glow step failed: %s", esp_err_to_name(err));
            }
        }
    }
}

//---------------------------------------------------------------------------------------------//

/* Basic LED functions */

esp_err_t led_init(led_t *led)
{
    if (!led)
        return ESP_ERR_INVALID_ARG;

    if (!GPIO_IS_VALID_OUTPUT_GPIO(led->pin))
        return ESP_ERR_INVALID_ARG;

    led->toggle_on = false;
    led->mode = LED_MODE_OFF;
    led->pwm_initialized = false;
    led->period_ms = 1000;
    led->worker_task = NULL;
    led->cmd_q = NULL;
    led->last_error = ESP_OK;

    gpio_config_t pin_config = {
        .pin_bit_mask = (1ULL << led->pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};

    ESP_RETURN_ON_ERROR(gpio_config(&pin_config), TAG, "gpio_config failed");
    ESP_RETURN_ON_ERROR(gpio_set_level(led->pin, 0), TAG, "gpio_set_level failed");

    led->cmd_q = xQueueCreate(1, sizeof(led_cmd_t));
    if (!led->cmd_q)
    {
        ESP_LOGE(TAG, "failed to create command queue");
        return ESP_ERR_NO_MEM;
    }

    BaseType_t res = xTaskCreate(
        led_worker,
        "led worker",
        2048,
        led,
        LED_WORKER_TASK_PRIORITY,
        &led->worker_task);

    if (res != pdPASS)
    {
        vQueueDelete(led->cmd_q);
        led->cmd_q = NULL;
        ESP_LOGE(TAG, "failed to create worker task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t led_on(led_t *led)
{
    if (!led)
        return ESP_ERR_INVALID_ARG;

    if (!led->cmd_q || !led->worker_task)
        return ESP_ERR_INVALID_STATE;

    led_cmd_t cmd = {
        .type = LED_CMD_SET_MODE,
        .mode = LED_MODE_ON,
        .brightness = 0,
        .period_ms = 0};

    BaseType_t ok = xQueueOverwrite(led->cmd_q, &cmd);
    if (ok != pdPASS)
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t led_off(led_t *led)
{
    if (!led)
        return ESP_ERR_INVALID_ARG;

    if (!led->cmd_q || !led->worker_task)
        return ESP_ERR_INVALID_STATE;

    led_cmd_t cmd = {
        .type = LED_CMD_SET_MODE,
        .mode = LED_MODE_OFF,
        .brightness = 0,
        .period_ms = 0};

    BaseType_t ok = xQueueOverwrite(led->cmd_q, &cmd);
    if (ok != pdPASS)
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t led_blink(led_t *led, uint32_t period)
{
    if (!led)
        return ESP_ERR_INVALID_ARG;

    // Validate period is within acceptable range
    if (period < LED_MIN_PERIOD_MS || period > LED_MAX_PERIOD_MS)
    {
        ESP_LOGE(TAG, "Blink period %lu ms out of valid range [%u, %u] ms",
                 period, LED_MIN_PERIOD_MS, LED_MAX_PERIOD_MS);
        return ESP_ERR_INVALID_ARG;
    }

    if (!led->cmd_q || !led->worker_task)
        return ESP_ERR_INVALID_STATE;

    led_cmd_t cmd = {
        .type = LED_CMD_SET_MODE,
        .mode = LED_MODE_BLINK,
        .brightness = 0,
        .period_ms = period};

    BaseType_t ok = xQueueOverwrite(led->cmd_q, &cmd);
    if (ok != pdPASS)
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t led_deinit(led_t *led)
{
    if (!led)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Send STOP command to worker task BEFORE deleting the queue
    if (led->cmd_q != NULL && led->worker_task != NULL)
    {
        led_cmd_t cmd = {
            .type = LED_CMD_STOP,
            .mode = LED_MODE_OFF,
            .brightness = 0,
            .period_ms = 0};

        BaseType_t ok = xQueueOverwrite(led->cmd_q, &cmd);
        if (ok != pdPASS)
        {
            ESP_LOGW(TAG, "Failed to send STOP command to worker");
        }

        // Give worker a brief moment to shut down gracefully
        // The worker task will call vTaskDelete(NULL) when it receives STOP
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Delete command queue (safe to call on NULL)
    if (led->cmd_q != NULL)
    {
        vQueueDelete(led->cmd_q);
        led->cmd_q = NULL;
    }

    // Turn off hardware directly
    // For PWM mode: stop LEDC first, then GPIO
    if (led->pwm_initialized)
    {
        esp_err_t err = ledc_set_duty(LEDC_LOW_SPEED_MODE, led->channel, 0);
        if (err != ESP_OK)
            ESP_LOGW(TAG, "Failed to set PWM duty to zero: %s", esp_err_to_name(err));

        err = ledc_update_duty(LEDC_LOW_SPEED_MODE, led->channel);
        if (err != ESP_OK)
            ESP_LOGW(TAG, "Failed to update PWM duty: %s", esp_err_to_name(err));

        // Stop the LEDC timer to fully disable PWM
        err = ledc_timer_pause(LEDC_LOW_SPEED_MODE, led->timer);
        if (err != ESP_OK)
            ESP_LOGW(TAG, "Failed to pause LEDC timer: %s", esp_err_to_name(err));
    }
    else
    {
        // For GPIO mode: just set level to 0
        esp_err_t err = gpio_set_level(led->pin, 0);
        if (err != ESP_OK)
            ESP_LOGW(TAG, "Failed to set GPIO low: %s", esp_err_to_name(err));
    }

    // Reset runtime/config state
    led->mode = LED_MODE_OFF;
    led->period_ms = 1000;
    led->toggle_on = false;
    led->pwm_initialized = false;
    led->worker_task = NULL;
    led->last_error = ESP_OK;

    return ESP_OK;
}

//--------------------------------------------------------------------------//

/* PWM */
/* Basic PWM functions */

esp_err_t led_pwm_init(led_t *led)
{
    if (!led)
        return ESP_ERR_INVALID_ARG;

    if (!GPIO_IS_VALID_OUTPUT_GPIO(led->pin))
        return ESP_ERR_INVALID_ARG;

    if (led->pwm_initialized)
    {
        ESP_LOGE(TAG, "PWM ALREADY INITIALIZED");
        return ESP_ERR_INVALID_STATE;
    }

    // Validate channel - must be explicitly set before calling this function
    if (led->channel >= LEDC_CHANNEL_MAX)
    {
        ESP_LOGE(TAG, "Invalid LEDC channel: %d (valid range: 0-%d). "
                      "You must set led->channel before calling led_pwm_init()",
                 led->channel, LEDC_CHANNEL_MAX - 1);
        return ESP_ERR_INVALID_ARG;
    }

    // Validate timer - must be explicitly set before calling this function
    if (led->timer >= LEDC_TIMER_MAX)
    {
        ESP_LOGE(TAG, "Invalid LEDC timer: %d (valid range: 0-%d). "
                      "You must set led->timer before calling led_pwm_init()",
                 led->timer, LEDC_TIMER_MAX - 1);
        return ESP_ERR_INVALID_ARG;
    }

    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LED_PWM_DUTY_RESOLUTION,
        .timer_num = led->timer,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK};

    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer), TAG, "PWM LED TIMER CONFIG FAILED");

    ledc_channel_config_t channel = {
        .gpio_num = led->pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = led->channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = led->timer,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD};

    ESP_RETURN_ON_ERROR(ledc_channel_config(&channel), TAG, "PWM LED CHANNEL CONFIG FAILED");

    led->pwm_initialized = true;

    return ESP_OK;
}

esp_err_t led_glow(led_t *led, uint32_t period)
{
    if (!led)
        return ESP_ERR_INVALID_ARG;

    // Validate period is within acceptable range
    if (period < LED_MIN_PERIOD_MS || period > LED_MAX_PERIOD_MS)
    {
        ESP_LOGE(TAG, "Glow period %lu ms out of valid range [%u, %u] ms",
                 period, LED_MIN_PERIOD_MS, LED_MAX_PERIOD_MS);
        return ESP_ERR_INVALID_ARG;
    }

    if (!led->cmd_q || !led->worker_task || !led->pwm_initialized)
        return ESP_ERR_INVALID_STATE;

    led_cmd_t cmd = {
        .type = LED_CMD_SET_MODE,
        .mode = LED_MODE_GLOW,
        .brightness = 0,
        .period_ms = period};

    BaseType_t ok = xQueueOverwrite(led->cmd_q, &cmd);
    if (ok != pdPASS)
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t led_set_brightness(led_t *led, uint8_t brightness)
{
    if (!led)
        return ESP_ERR_INVALID_ARG;

    if (!led->cmd_q || !led->worker_task || !led->pwm_initialized)
        return ESP_ERR_INVALID_STATE;

    led_cmd_t cmd = {
        .type = LED_CMD_SET_BRIGHTNESS,
        .mode = LED_MODE_ON,
        .brightness = brightness,
        .period_ms = 0};

    BaseType_t ok = xQueueOverwrite(led->cmd_q, &cmd);
    if (ok != pdPASS)
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}