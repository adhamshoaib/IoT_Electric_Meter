#include "voltage_sensor.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>

#define TAG "VOLTAGE_SENSOR"

// ─── internal context ────────────────────────────────────────────────────────

struct voltage_sensor_ctx {
    adc_channel_t    channel;

    // ── accumulator (written from ADC task) ──
    float            sum_sq;          // running sum of (v - offset)^2
    float            peak_sq;         // peak |v| seen this window
    uint32_t         sample_count;    // samples accumulated so far

    // ── instantaneous (written from ADC task) ──
    float            v_inst;          // latest DC-removed, scaled voltage (V)

    // ── result (protected by mutex) ──
    voltage_sensor_result_t   result;
    SemaphoreHandle_t mutex;
};

// ─── helpers ─────────────────────────────────────────────────────────────────

/**
 * Convert a raw ADC millivolt reading to a real-world, DC-removed voltage.
 *
 *   1. Subtract DC bias (midpoint of the ADC input range for AC signals)
 *   2. Convert mV → V
 *   3. Apply scale factor (voltage divider / conditioning circuit ratio)
 */
static inline float s_to_real_volts(float raw_mv)
{
    return ((raw_mv - VOLTAGE_SENSOR_DC_OFFSET_MV) / 1000.0f) * VOLTAGE_SENSOR_SCALE_FACTOR;
}

// ─── public API ──────────────────────────────────────────────────────────────

esp_err_t voltage_sensor_init(adc_channel_t channel, voltage_sensor_ctx_t **out_ctx)
{
    if (out_ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    voltage_sensor_ctx_t *ctx = calloc(1, sizeof(voltage_sensor_ctx_t));
    if (ctx == NULL) {
        ESP_LOGE(TAG, "Failed to allocate sensor context");
        return ESP_ERR_NO_MEM;
    }

    ctx->channel = channel;

    ctx->mutex = xSemaphoreCreateMutex();
    if (ctx->mutex == NULL) {
        free(ctx);
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    ctx->result.valid = false;

    ESP_LOGI(TAG, "Voltage sensor initialised on ADC channel %d", (int)channel);

    *out_ctx = ctx;
    return ESP_OK;
}

void voltage_sensor_feed(voltage_sensor_ctx_t *ctx, float raw_mv)
{
    if (ctx == NULL) return;

    // ── 1. Convert to real-world, DC-removed voltage ──
    float v = s_to_real_volts(raw_mv);

    // ── 2. Store for instantaneous power use ──
    ctx->v_inst = v;

    // ── 3. Accumulate sum of squares and track peak ──
    ctx->sum_sq += v * v;

    float v_abs = (v < 0.0f) ? -v : v;
    if (v_abs > ctx->peak_sq) {
        ctx->peak_sq = v_abs;
    }

    ctx->sample_count++;

    // ── 4. Recompute Vrms when window is full ──
    if (ctx->sample_count >= VOLTAGE_SENSOR_WINDOW_SIZE) {

        float vrms_v  = sqrtf(ctx->sum_sq / (float)ctx->sample_count);
        float vpeak_v = ctx->peak_sq;
        uint32_t n    = ctx->sample_count;

        // Update result under mutex — result is read from another task
        if (xSemaphoreTake(ctx->mutex, 0) == pdTRUE) {
            ctx->result.vrms_v         = vrms_v;
            ctx->result.vpeak_v        = vpeak_v;
            ctx->result.window_samples = n;
            ctx->result.valid          = true;
            xSemaphoreGive(ctx->mutex);
        }
        // If mutex is held by reader, skip this update — next window will catch up.
        // This is intentional: voltage_sensor_feed must never block (it's called from ADC task).

        // Reset accumulator for next window
        ctx->sum_sq       = 0.0f;
        ctx->peak_sq      = 0.0f;
        ctx->sample_count = 0;
    }
}

void voltage_sensor_get_result(const voltage_sensor_ctx_t *ctx, voltage_sensor_result_t *out)
{
    if (ctx == NULL || out == NULL) return;

    if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        *out = ctx->result;
        xSemaphoreGive(ctx->mutex);
    } else {
        // Return invalid result on timeout rather than stale/corrupt data
        memset(out, 0, sizeof(voltage_sensor_result_t));
        out->valid = false;
    }
}

float voltage_sensor_get_instantaneous(const voltage_sensor_ctx_t *ctx)
{
    if (ctx == NULL) return 0.0f;
    // v_inst is a float — single word, atomic read on Xtensa.
    // No mutex needed for this fast path.
    return ctx->v_inst;
}

void voltage_sensor_reset(voltage_sensor_ctx_t *ctx)
{
    if (ctx == NULL) return;

    if (xSemaphoreTake(ctx->mutex, portMAX_DELAY) == pdTRUE) {
        ctx->sum_sq       = 0.0f;
        ctx->peak_sq      = 0.0f;
        ctx->sample_count = 0;
        ctx->v_inst       = 0.0f;
        ctx->result.valid = false;
        xSemaphoreGive(ctx->mutex);
    }
}

void voltage_sensor_deinit(voltage_sensor_ctx_t *ctx)
{
    if (ctx == NULL) return;
    vSemaphoreDelete(ctx->mutex);
    free(ctx);
}