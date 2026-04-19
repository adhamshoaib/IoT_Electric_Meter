/**
 * @file voltage_sensor.h
 * @brief Stateless conversion helpers for ZMPT101B voltage sensing.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @brief DC bias on the ADS1115 input in millivolts.
 *
 * Nominally Vcc/2 = 1650 mV on a 3.3V system.
 */
#define VOLTAGE_SENSOR_BIAS_MV 1650.0f

/**
 * @brief ZMPT101B scale factor from bias-removed mV to mains volts.
 *
 * Derivation:
 * scale = V_mains_peak / V_sensor_peak_after_bias_removal
 */
#define VOLTAGE_SENSOR_SCALE_FACTOR 0.3415f

/**
 * @brief Convert a single ADS1115 millivolt reading to calibrated mains voltage.
 *
 * Removes the DC bias and applies the ZMPT101B scale factor to produce
 * an instantaneous voltage sample in volts.
 *
 * @param mv Raw millivolt reading from the ADS1115 path.
 * @return Instantaneous voltage in volts, centered around 0V.
 */
float voltage_sensor_convert(int16_t mv);

/**
 * @brief Compute RMS voltage from a window of ADS1115 millivolt samples.
 *
 * Applies voltage_sensor_convert() to each sample and computes discrete RMS.
 *
 * @param mv_samples Pointer to array of raw millivolt readings.
 * @param count Number of samples in the array.
 * @return RMS voltage in volts, or 0.0f if input is invalid.
 */
float voltage_sensor_rms(const int16_t *mv_samples, size_t count);
