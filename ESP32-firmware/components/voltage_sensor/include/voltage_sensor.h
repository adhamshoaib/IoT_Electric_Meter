/**
 * @file voltage_sensor.h
 * @brief Stateless conversion helpers for ZMPT101B voltage sensing.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Time-aligned raw ADC sample pair.
 */
typedef struct
{
    int16_t v_raw;
    int16_t i_raw;
} adc_sample_pair_t;

/**
 * @brief DC bias on the ADS1115 voltage channel in raw counts.
 *
 * Nominally Vcc/2 = 1650 mV on a 3.3V system.
 * With ADS1115 gain set to +/-4.096V (0.125 mV/count), bias is 13200 counts.
 */
#define VOLTAGE_SENSOR_BIAS_RAW 13200.0f

/**
 * @brief ADS1115 scaling for the voltage channel in mV/count.
 *
 * This assumes gain is configured as ADS_FSR_4_096V for voltage sampling.
 */
#define VOLTAGE_SENSOR_RAW_TO_MV (4.096f * 1000.0f / 32768.0f)

/**
 * @brief ZMPT101B scale factor from bias-removed mV to mains volts.
 *
 * Derivation:
 * scale = V_mains_peak / V_sensor_peak_after_bias_removal
 */
#define VOLTAGE_SENSOR_SCALE_FACTOR 0.3415f

/**
 * @brief Combined scale factor from raw ADC counts to mains volts.
 */
#define VOLTAGE_SENSOR_RAW_TO_VOLTS (VOLTAGE_SENSOR_RAW_TO_MV * VOLTAGE_SENSOR_SCALE_FACTOR)

/**
 * @brief Convert one raw ADS1115 sample to calibrated mains voltage.
 *
 * Removes the DC bias in raw-count space and applies the counts-to-volts scale
 * to produce an instantaneous voltage sample in volts.
 *
 * @param raw Raw ADS1115 count from the voltage channel.
 * @return Instantaneous voltage in volts, centered around 0V.
 */
float voltage_sensor_convert(int16_t raw);

/**
 * @brief Compute RMS voltage from a window of paired raw ADC samples.
 *
 * Applies voltage_sensor_convert() to each pair's v_raw sample and computes
 * discrete RMS.
 *
 * @param pairs Pointer to array of time-aligned raw voltage/current pairs.
 * @param count Number of samples in the array.
 * @return RMS voltage in volts, or 0.0f if input is invalid.
 */
float voltage_sensor_rms(const adc_sample_pair_t *pairs, size_t count);
