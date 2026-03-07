#pragma once

#include "esp_err.h"
#include "adc_cont.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ─── tunables ────────────────────────────────────────────────────────────────

// Samples per Irms window.
// At 20 kHz, 400 samples = exactly one 50 Hz cycle.
#define CURRENT_SENSOR_WINDOW_SIZE      400

// SCT013-030 electrical chain:
//   Turns ratio        : 1800:1
//   Internal burden    : ~60Ω  (datasheet: 30A primary → 1V RMS output
//                               → R = V / I_sec = 1.0 / (30/1800) = 60Ω)
//   Full-scale output  : 1V RMS at 30A primary
//
// PRIMARY_TURNS: how many times the measured wire passes through the CT.
//   For high-current loads (>1A): 1 turn is fine.
//   For low-current loads (<1A, e.g. a 40W bulb at 220V = 0.18A):
//     increase turns to multiply effective primary current.
//     e.g. 10 turns → effective primary = 0.18A × 10 = 1.8A (readable)
//     Then actual current = reported_current / PRIMARY_TURNS.
//
// Scale factor = (turns_ratio / R_burden) / primary_turns
#define CURRENT_SENSOR_TURNS_RATIO      1800
#define CURRENT_SENSOR_BURDEN_OHM       60.0f
#define CURRENT_SENSOR_PRIMARY_TURNS    4       // wire passes through CT 4 times
#define CURRENT_SENSOR_SCALE_FACTOR     (CURRENT_SENSOR_TURNS_RATIO                                          / CURRENT_SENSOR_BURDEN_OHM                                          / CURRENT_SENSOR_PRIMARY_TURNS)

// DC bias at the ADC pin (midpoint of the AC swing).
// With R1=R2=10kΩ on a 3.3V supply: 3300 / 2 = 1650 mV.
// IMPORTANT: measure the actual voltage at the ADC pin with no load
// and set this to that value. Even a 50mV error at scale=7.5 causes
// 0.075A of residual noise (at scale=30 that's 0.3A).
// Use current_sensor_calibrate_offset() at startup with no load to
// auto-measure this value at runtime.
#define CURRENT_SENSOR_DC_OFFSET_MV     1650.0f

// Valid ADC input range at ADC_ATTEN_DB_12 on ESP32.
#define CURRENT_SENSOR_ADC_MIN_MV       0.0f
#define CURRENT_SENSOR_ADC_MAX_MV       3100.0f

// Sanity ceiling — SCT013-030 is rated 30A, allow 20% headroom.
#define CURRENT_SENSOR_MAX_IRMS_A       36.0f

// Noise floor: minimum Irms reported as non-zero.
// ADC noise around the DC offset produces a residual ~0.05–0.15A
// even with no load. Readings below this threshold are clamped to 0.
// Measure with no load connected and set slightly above that reading.
#define CURRENT_SENSOR_NOISE_FLOOR_A    0.1f

// Calibration correction factor — corrects for CT burden tolerance,
// ADC nonlinearity, and real-world component variation.
//
// How to set this:
//   1. Connect a known resistive load.
//   2. Measure true current with a clamp meter → I_true.
//   3. Read what this driver reports               → I_reported.
//   4. CAL_FACTOR = I_true / I_reported
//
// Example: clamp meter reads 0.18A, driver reports 0.11A
//          CAL_FACTOR = 0.18 / 0.11 = 1.636
//
// Leave at 1.0f until you have a clamp meter reading.
#define CURRENT_SENSOR_CAL_FACTOR       1.6f

// ─── result ──────────────────────────────────────────────────────────────────

typedef struct {
    float    irms_a;            ///< RMS current in Amps
    float    ipeak_a;           ///< Peak current seen in this window
    uint32_t window_samples;    ///< Samples that went into this result
    bool     valid;             ///< false until first full window completes
} current_sensor_result_t;

// ─── handle ──────────────────────────────────────────────────────────────────

typedef struct current_sensor_ctx current_sensor_ctx_t;

// ─── API ─────────────────────────────────────────────────────────────────────

/**
 * @brief Allocate and initialise a current sensor context.
 *
 * @param channel   ADC channel this sensor listens to.
 * @param out_ctx   Receives the allocated context pointer.
 * @return ESP_OK on success.
 */
esp_err_t current_sensor_init(adc_channel_t channel, current_sensor_ctx_t **out_ctx);

/**
 * @brief Feed one calibrated ADC sample (mV) into the sensor.
 *
 * Call from your ADC callback for samples matching this sensor's channel.
 * Safe to call from ADC task context — no heap allocation, no blocking.
 *
 * @param ctx     Sensor context.
 * @param raw_mv  Calibrated ADC reading in millivolts.
 */
void current_sensor_feed(current_sensor_ctx_t *ctx, float raw_mv);

/**
 * @brief Get the latest computed Irms result.
 *
 * @param ctx   Sensor context.
 * @param out   Filled with the latest result.
 */
void current_sensor_get_result(const current_sensor_ctx_t *ctx, current_sensor_result_t *out);

/**
 * @brief Get the latest instantaneous current sample in Amps.
 *
 * Intended for instantaneous power calculation:
 *   p(t) = voltage_sensor_get_instantaneous(v_ctx)
 *          * current_sensor_get_instantaneous(i_ctx)
 *
 * @param ctx   Sensor context.
 * @return      Instantaneous current in Amps. 0.0f if no sample yet.
 */
float current_sensor_get_instantaneous(const current_sensor_ctx_t *ctx);

/**
 * @brief Calibrate the DC offset by averaging raw ADC samples with no load.
 *
 * Call ONCE at startup before connecting any load. Samples the ADC for
 * `num_samples` readings and sets the internal DC offset to their mean.
 * This corrects for bias resistor tolerances and ADC nonlinearity.
 *
 * The measured offset is printed via ESP_LOGI so you can copy it into
 * CURRENT_SENSOR_DC_OFFSET_MV for a permanent fix.
 *
 * @param ctx           Sensor context.
 * @param num_samples   How many samples to average (recommend >= 2000,
 *                      = 100ms at 20kHz, covers multiple full cycles).
 */
void current_sensor_calibrate_offset(current_sensor_ctx_t *ctx, uint32_t num_samples);

/**
 * @brief Check whether calibration has completed.
 *
 * Call from your application task (NOT from the ADC callback).
 * When it returns true it logs the measured offset and resets
 * the accumulator so normal Irms computation can begin immediately.
 *
 * @param ctx   Sensor context.
 * @return      true once calibration is complete, false while still running.
 */
bool current_sensor_calibration_done(current_sensor_ctx_t *ctx);

/**
 * @brief Reset accumulator and invalidate result. Does not free memory.
 * @param ctx   Sensor context.
 */
void current_sensor_reset(current_sensor_ctx_t *ctx);

/**
 * @brief Free the sensor context.
 * @param ctx   Sensor context.
 */
void current_sensor_deinit(current_sensor_ctx_t *ctx);

#ifdef __cplusplus
}
#endif