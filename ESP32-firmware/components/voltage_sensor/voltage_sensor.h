#pragma once

#include "esp_err.h"
#include "adc_cont.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// ─── tunables ────────────────────────────────────────────────────────────────

// Number of ADC samples accumulated before Vrms is recomputed.
// For 50 Hz mains: 20000 / 50 = 400 samples = exactly one cycle.
#define VOLTAGE_SENSOR_WINDOW_SIZE      400

// Voltage divider + signal conditioning scale factor.
// actual_voltage = (adc_mv / 1000.0) * VOLTAGE_SENSOR_SCALE_FACTOR
#define VOLTAGE_SENSOR_SCALE_FACTOR     341.5f    // set from Phase 1 diagnostics

// DC bias at the ADC pin (midpoint of AC waveform).
// Use voltage_sensor_calibrate_offset() at startup to measure this
// automatically, then copy the logged value here for permanent use.
#define VOLTAGE_SENSOR_DC_OFFSET_MV     1650.0f

// Valid ADC input range at ADC_ATTEN_DB_12 on ESP32.
#define VOLTAGE_SENSOR_ADC_MIN_MV       0.0f
#define VOLTAGE_SENSOR_ADC_MAX_MV       3100.0f

// Sanity ceiling — Vrms above this is marked invalid.
#define VOLTAGE_SENSOR_MAX_VRMS_V       300.0f

// ─── result ──────────────────────────────────────────────────────────────────

typedef struct {
    float    vrms_v;            ///< RMS voltage in Volts (scaled to real-world)
    float    vpeak_v;           ///< Peak voltage seen in this window
    uint32_t window_samples;    ///< Samples that went into this result
    bool     valid;             ///< false until first full window completes
} voltage_sensor_result_t;

// ─── handle ──────────────────────────────────────────────────────────────────

typedef struct voltage_sensor_ctx voltage_sensor_ctx_t;

// ─── API ─────────────────────────────────────────────────────────────────────

/**
 * @brief Allocate and initialise a voltage sensor context.
 *
 * @param channel   ADC channel this sensor listens to.
 * @param out_ctx   Receives the allocated context pointer.
 * @return ESP_OK on success.
 */
esp_err_t voltage_sensor_init(adc_channel_t channel, voltage_sensor_ctx_t **out_ctx);

/**
 * @brief Feed one calibrated ADC sample (mV) into the sensor.
 *
 * Call from your ADC callback for samples matching this sensor's channel.
 * Safe to call from ADC task context — no heap allocation, no blocking.
 *
 * @param ctx       Sensor context.
 * @param raw_mv    Calibrated ADC reading in millivolts.
 */
void voltage_sensor_feed(voltage_sensor_ctx_t *ctx, float raw_mv);

/**
 * @brief Get the latest computed Vrms result.
 *
 * @param ctx   Sensor context.
 * @param out   Filled with the latest result. valid=false until first window.
 */
void voltage_sensor_get_result(const voltage_sensor_ctx_t *ctx, voltage_sensor_result_t *out);

/**
 * @brief Get the latest instantaneous voltage in Volts.
 *
 * DC-removed and scaled. Use for instantaneous power:
 *   p(t) = voltage_sensor_get_instantaneous(v) * current_sensor_get_instantaneous(i)
 *
 * @param ctx   Sensor context.
 * @return      Instantaneous voltage in Volts. 0.0f if no sample yet.
 */
float voltage_sensor_get_instantaneous(const voltage_sensor_ctx_t *ctx);

/**
 * @brief Start DC offset calibration.
 *
 * Call with the ZMPT101B primary disconnected from mains (no AC signal).
 * Calibration runs asynchronously inside voltage_sensor_feed().
 * Poll voltage_sensor_calibration_done() to know when it completes.
 *
 * @param ctx           Sensor context.
 * @param num_samples   Samples to average — recommend >= 2000 (100ms at 20kHz).
 */
void voltage_sensor_calibrate_offset(voltage_sensor_ctx_t *ctx, uint32_t num_samples);

/**
 * @brief Check whether calibration has completed.
 *
 * Call this from your application task (NOT from the ADC callback).
 * When it returns true it also logs the measured offset and resets
 * the accumulator so normal Vrms computation can begin immediately.
 *
 * @param ctx   Sensor context.
 * @return      true once calibration is complete, false while still running.
 */
bool voltage_sensor_calibration_done(voltage_sensor_ctx_t *ctx);

/**
 * @brief Reset accumulator and invalidate result. Does not free memory.
 * @param ctx   Sensor context.
 */
void voltage_sensor_reset(voltage_sensor_ctx_t *ctx);

/**
 * @brief Free the sensor context.
 * @param ctx   Sensor context.
 */
void voltage_sensor_deinit(voltage_sensor_ctx_t *ctx);

#ifdef __cplusplus
}
#endif