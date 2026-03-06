#ifndef VOLTAGE_SENSOR.H
#define VOLTAGE_SENSOR.H

#include "esp_err.h"
#include "adc_cont.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// ─── tunables ────────────────────────────────────────────────────────────────

// Number of ADC samples accumulated before Vrms is recomputed.
// At 20 kHz, 200 samples = 10 ms window = one full cycle at 100 Hz (conservative).
// For 50 Hz mains: 20000 / 50 = 400 samples = exactly one cycle.
#define VOLTAGE_SENSOR_WINDOW_SIZE     400

// Voltage divider + signal conditioning scale factor:
// actual_voltage = (adc_mv / 1000.0) * VOLTAGE_SENSOR_SCALE_FACTOR
// Set this to match your resistor divider ratio.
// Example: if you divided 325Vpeak down to 3.3V → scale = 325.0 / 3.3 ≈ 98.5
#define VOLTAGE_SENSOR_SCALE_FACTOR    341.5f   // override for your circuit

// DC offset of the ADC signal in mV (midpoint of AC waveform on ADC input).
// For a biased AC signal centered at Vcc/2 on a 3.3V supply: ~1650 mV.
// Set to 0 if your signal is already DC-free.
#define VOLTAGE_SENSOR_DC_OFFSET_MV    2500.0f

// ─── result type ─────────────────────────────────────────────────────────────

typedef struct {
    float    vrms_v;            ///< RMS voltage in Volts (scaled to real-world)
    float    vpeak_v;           ///< Peak voltage seen in this window
    uint32_t window_samples;    ///< How many samples went into this result
    bool     valid;             ///< false until first full window is complete
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
 * @brief Feed one calibrated ADC sample (in mV) into the sensor.
 *
 * Call this from your ADC callback for samples that match this sensor's
 * channel.  Internally accumulates sum-of-squares; Vrms is recomputed
 * automatically every voltage_sensor_WINDOW_SIZE samples.
 *
 * Safe to call from ADC task context — no heap allocation, no blocking.
 *
 * @param ctx       Sensor context.
 * @param raw_mv    Calibrated ADC reading in millivolts.
 */
void voltage_sensor_feed(voltage_sensor_ctx_t *ctx, float raw_mv);

/**
 * @brief Get the latest computed Vrms result.
 *
 * @param ctx    Sensor context.
 * @param out    Filled with the latest result. `valid` is false until
 *               the first window completes.
 */
void voltage_sensor_get_result(const voltage_sensor_ctx_t *ctx, voltage_sensor_result_t *out);

/**
 * @brief Get the latest instantaneous voltage sample in Volts.
 *
 * This is the most recent DC-removed, scaled voltage value.
 * Intended for use in instantaneous power calculation:
 *   p(t) = v_inst(t) * i_inst(t)
 *
 * @param ctx   Sensor context.
 * @return      Instantaneous voltage in Volts. 0.0f if no sample yet.
 */
float voltage_sensor_get_instantaneous(const voltage_sensor_ctx_t *ctx);

/**
 * @brief Reset accumulator and result. Does not free memory.
 * @param ctx   Sensor context.
 */
void voltage_sensor_reset(voltage_sensor_ctx_t *ctx);

/**
 * @brief Free the sensor context.
 * @param ctx   Sensor context.
 */
void voltage_sensor_deinit(voltage_sensor_ctx_t *ctx);

#endif