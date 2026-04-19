/**
 * @file current_sensor.h
 * @brief Stateless conversion helpers for current sensing.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @brief DC bias on the ADS1115 input in millivolts.
 *
 * Differential current input is centered around 0 mV.
 */
#define CURRENT_SENSOR_BIAS_MV 0.0f

/**
 * @brief Current sensor scale factor from differential mV to amps.
 *
 * Theoretical for 30A/1V is 0.03 A/mV.
 * This project uses a calibrated value from a known-load measurement.
 */
#define CURRENT_SENSOR_SCALE_FACTOR 0.01424f

/**
 * @brief Convert one ADS1115 millivolt reading to instantaneous current.
 *
 * Removes the DC bias and applies the current scale factor.
 *
 * @param mv Raw millivolt reading from the ADS1115 path.
 * @return Instantaneous current in amps, centered around 0A.
 */
float current_sensor_convert(int16_t mv);

/**
 * @brief Compute RMS current from a window of ADS1115 millivolt samples.
 *
 * Applies current_sensor_convert() to each sample and computes discrete RMS.
 *
 * @param mv_samples Pointer to array of raw millivolt readings.
 * @param count Number of samples in the array.
 * @return RMS current in amps, or 0.0f if input is invalid.
 */
float current_sensor_rms(const int16_t *mv_samples, size_t count);
