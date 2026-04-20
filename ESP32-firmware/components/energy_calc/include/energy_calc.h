/**
 * @file energy_calc.h
 * @brief Stateless power and energy math for converted voltage/current samples.
 */

#pragma once

#include <stddef.h>

#include "esp_err.h"

typedef struct
{
    float real_power_w;
    float apparent_power_va;
    float power_factor;
} power_calc_result_t;

/**
 * @brief Compute real power, apparent power, and power factor for one sample window.
 *
 * Caller contract:
 * - v_samples and i_samples must have the same count and be time-aligned pairs.
 * - Inputs must already be converted to physical units (V and A).
 * - Inputs may include residual DC offset; this function removes per-window
 *   means internally before computing power terms.
 * - Do not pass interleaved pair buffers directly.
 *
 * The function removes residual DC offset internally using per-window mean subtraction.
 *
 * @param[out] out Result struct. Populated on success, zeroed on error when non-NULL.
 * @param[in] v_samples Voltage samples in volts.
 * @param[in] i_samples Current samples in amps.
 * @param[in] count Number of samples in each array.
 * @param[in] current_advance_samples Current-signal shift in sample units.
 *            Positive values move toward later sample indices, negative toward
 *            earlier indices.
 * @param[in] voltage_advance_samples Voltage-signal shift in sample units.
 *            Positive values move toward later sample indices, negative toward
 *            earlier indices.
 *
 *            Fractional values are supported via linear interpolation.
 *            Tune one side at a time with small values (e.g., -1.0 to 1.0).
 *
 * @return ESP_OK on success.
 * @return ESP_ERR_INVALID_ARG if out, v_samples, or i_samples is NULL,
 *         if count is less than 2, or if either phase shift is invalid.
 *
 * Note: negative real power is treated as an invalid measurement and clamped
 * to 0 W before computing power_factor.
 */
esp_err_t energy_calc_power(power_calc_result_t *out,
                            const float *v_samples,
                            const float *i_samples,
                            size_t count,
                            float current_advance_samples,
                            float voltage_advance_samples);

/**
 * @brief Add one real-power window into a running kWh accumulator.
 *
 * Formula: delta_kwh = real_power_w * window_duration_s / 3600000
 *
 * Negative real_power_w is treated as an invalid measurement and clamped
 * to 0 W before accumulation.
 *
 * @param[in,out] accumulator_kwh Pointer to the running kWh total.
 * @param[in] real_power_w Real power for this window in watts.
 * @param[in] window_duration_s Measurement window duration in seconds (> 0).
 *
 * @return ESP_OK on success.
 * @return ESP_ERR_INVALID_ARG if accumulator_kwh is NULL or if
 *         window_duration_s <= 0.
 */
esp_err_t energy_calc_accumulate_kwh(float *accumulator_kwh,
                                     float real_power_w,
                                     float window_duration_s);
