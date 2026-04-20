/**
 * @file current_sensor.h
 * @brief Stateless conversion helpers for current sensing.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @brief DC bias on the ADS1115 differential current channel in raw counts.
 *
 * Differential current input is centered around 0 counts.
 */
#define CURRENT_SENSOR_BIAS_RAW 0.0f

/**
 * @brief ADS1115 scaling for the current channel in mV/count.
 *
 * This assumes gain is configured as ADS_FSR_1_024V for current sampling.
 */
#define CURRENT_SENSOR_RAW_TO_MV (1.024f * 1000.0f / 32768.0f)

/**
 * @brief Current scale in amperes per millivolt (A/mV).
 *
 * Theoretical SCT-013 30A/1V transfer is 0.03 A/mV.
 * This project uses a calibrated value from a known-load measurement.
 *
 * IMPORTANT:
 * Recalibrate this value if CT model, burden resistor, analog front-end gain,
 * or ADS1115 gain setting changes. Any error here scales RMS current, power,
 * and energy by the same factor.
 */
#define CURRENT_SENSOR_SCALE_A_PER_MV 0.01424f

/**
 * @brief Current channel polarity correction.
 *
 * Set to +1.0f for normal orientation, -1.0f if CT leads/orientation invert
 * the measured current polarity. For home-meter mode, wrong polarity drives
 * real power negative and energy_calc_power() clamps it to 0 W.
 */
#define CURRENT_SENSOR_POLARITY -1.0f

/**
 * @brief Combined scale factor from raw ADC counts to amps.
 */
#define CURRENT_SENSOR_RAW_TO_AMPS (CURRENT_SENSOR_RAW_TO_MV * CURRENT_SENSOR_SCALE_A_PER_MV * CURRENT_SENSOR_POLARITY)

/**
 * @brief Backward-compatible alias for legacy naming.
 */
#define CURRENT_SENSOR_SCALE_FACTOR CURRENT_SENSOR_SCALE_A_PER_MV

/**
 * @brief Convert one raw ADS1115 sample to instantaneous signed current.
 *
 * Removes the DC bias in raw-count space and applies the counts-to-amps scale.
 *
 * This is a single-sample conversion only; it does not remove residual
 * per-window DC offset. Do not use this directly for instantaneous power
 * products unless the caller also removes the window mean.
 *
 * @param raw Raw ADS1115 differential count from the current channel.
 * @return Instantaneous current in amperes (A), nominally centered around 0A.
 */
float current_sensor_convert(int16_t raw);

/**
 * @brief Compute RMS current from a window of raw ADS1115 current samples.
 *
 * Applies current_sensor_convert() to each sample and computes discrete RMS.
 * Residual DC offset is removed internally using mean subtraction.
 *
 * Caller contract:
 * - raw_samples must contain current-channel samples only.
 *
 * @param raw_samples Pointer to array of raw ADS1115 counts.
 * @param count Number of samples in the array.
 * @return RMS current in amps, or 0.0f if input is invalid.
 */
float current_sensor_rms(const int16_t *raw_samples, size_t count);
