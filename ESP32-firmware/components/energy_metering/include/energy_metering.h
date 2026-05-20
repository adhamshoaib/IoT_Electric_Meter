#pragma once

#include "BL0939.h"
#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

/** Calibration and physical constants for one meter instance. */
typedef struct {
    /* Voltage channel */
    float vrms_scale;           /**< BL0939 Vrms full-scale count (default 79931) */
    float vref;                 /**< Internal Vref, volts (default 1.218) */
    float divider_ratio;        /**< External resistor-divider ratio (Vmains / Vpin) */
    float vac_fine_gain;        /**< Empirical trim for voltage channel */
    uint32_t vrms_zero_offset;  /**< ADC offset counts to subtract before scaling */
    float vac_noise_floor_v;    /**< Readings below this are clamped to 0 V */

    /* Current channel */
    float irms_scale;           /**< BL0939 Irms full-scale count (default 324004) */
    float ia_pin_fine_gain;     /**< Empirical trim for current sense amp */
    float ia_cal_a_per_mv;      /**< Current in A per mV at the sense pin */
    float ia_noise_floor_a;     /**< Readings below this are clamped to 0 A */
    float vp_noise_floor_mv;    /**< Sense-pin voltage noise floor, mV */

    /* Energy pulse counters */
    float energy_ref;           /**< BL0939 energy reference (default 3304) */
    float cf_count_scale;       /**< CF count scale factor (default 20000) */
} energy_metering_calibration_t;

/** Convenience initializer - matches the constants in the original main.c. */
#define ENERGY_METERING_CALIBRATION_DEFAULT() {     \
    .vrms_scale        = 79931.0f,                  \
    .vref              = 1.218f,                    \
    .divider_ratio     = (230.0f / 0.062f),         \
    .vac_fine_gain     = 0.956f,                    \
    .vrms_zero_offset  = 2720U,                     \
    .vac_noise_floor_v = 0.5f,                      \
    .irms_scale        = 324004.0f,                 \
    .ia_pin_fine_gain  = 0.686f,                    \
    .ia_cal_a_per_mv   = (0.4347f / 12.0f),         \
    .ia_noise_floor_a  = 0.003f,                    \
    .vp_noise_floor_mv = 0.2f,                      \
    .energy_ref        = 3304.0f,                   \
    .cf_count_scale    = 20000.0f,                  \
}

/** Driver configuration passed to energy_metering_init(). */
typedef struct {
    energy_metering_calibration_t calibration;
} energy_metering_config_t;

/** Processed measurement ready for application use. */
typedef struct {
    float voltage_v;        /**< RMS mains voltage, volts */
    float current_a;        /**< RMS load current, amps */
    float total_energy_kwh; /**< Accumulated energy since init or last reset, kWh */
} energy_metering_data_t;

/**
 * Initialise the energy_metering driver.
 *
 * Must be called after bl0939_init(). Does not touch hardware.
 * Resets the energy accumulator and counter state.
 *
 * @param config  Calibration and tuning parameters.
 * @return        ESP_OK on success, ESP_ERR_INVALID_ARG if config is NULL.
 */
esp_err_t energy_metering_init(const energy_metering_config_t *config);

/**
 * Read one measurement frame.
 *
 * Calls bl0939_read_raw() internally (respecting auto_request_before_read),
 * converts raw values to physical units, and accumulates energy.
 *
 * The first call after init seeds the CFA/CFB counters and returns 0 kWh;
 * subsequent calls return the delta since the previous call.
 *
 * @param[out] out  Populated on ESP_OK.
 * @param      timeout_ms  Passed to bl0939_read_raw(); use BL0939_TIMEOUT_USE_DEFAULT.
 * @return     ESP_OK, or propagates errors from bl0939_read_raw().
 */
esp_err_t energy_metering_read(energy_metering_data_t *out, uint32_t timeout_ms);

/**
 * Reset the accumulated energy counter to zero.
 *
 * The CFA/CFB baseline is re-seeded on the next energy_metering_read() call.
 * Safe to call from any task if external locking is provided.
 */
void energy_metering_reset_energy(void);
