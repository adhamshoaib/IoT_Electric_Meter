#include "current_sensor.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>

#define TAG "CURRENT_SENSOR"

// ─── internal context ────────────────────────────────────────────────────────

struct current_sensor_ctx {
    adc_channel_t     channel;
    float             dc_offset_mv;   // default from define, overridden by calibration

    // ── accumulator (written from ADC task) ──
    float             sum_sq;
    float             ipeak;
    uint32_t          sample_count;

    // ── instantaneous sample (written from ADC task) ──
    float             i_inst;

    // ── calibration state ──
    volatile bool     calibrating;    // true while feed() is collecting cal samples
    float             cal_sum;
    uint32_t          cal_count;
    uint32_t          cal_target;
    SemaphoreHandle_t cal_done_sem;   // feed() gives this when cal_target reached

    // ── result (mutex-protected) ──
    current_sensor_result_t              result;
    SemaphoreHandle_t mutex;
};

// ─── helpers ─────────────────────────────────────────────────────────────────

static inline float s_to_real_amps(const current_sensor_ctx_t *ctx, float raw_mv)
{
    return ((raw_mv - ctx->dc_offset_mv) / 1000.0f) * CURRENT_SENSOR_SCALE_FACTOR  * CURRENT_SENSOR_CAL_FACTOR;
}

// ─── public API ──────────────────────────────────────────────────────────────

esp_err_t current_sensor_init(adc_channel_t channel, current_sensor_ctx_t **out_ctx)
{
    if (out_ctx == NULL) return ESP_ERR_INVALID_ARG;

    current_sensor_ctx_t *ctx = calloc(1, sizeof(current_sensor_ctx_t));
    if (ctx == NULL) {
        ESP_LOGE(TAG, "Failed to allocate context");
        return ESP_ERR_NO_MEM;
    }

    ctx->channel      = channel;
    ctx->dc_offset_mv = CURRENT_SENSOR_DC_OFFSET_MV;

    ctx->mutex = xSemaphoreCreateMutex();
    if (ctx->mutex == NULL) {
        free(ctx);
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    ctx->cal_done_sem = xSemaphoreCreateBinary();
    if (ctx->cal_done_sem == NULL) {
        vSemaphoreDelete(ctx->mutex);
        free(ctx);
        ESP_LOGE(TAG, "Failed to create cal semaphore");
        return ESP_ERR_NO_MEM;
    }

    ctx->result.valid = false;
    ESP_LOGI(TAG, "Initialised on ADC channel %d, offset=%.1f mV",
             (int)channel, (double)ctx->dc_offset_mv);

    *out_ctx = ctx;
    return ESP_OK;
}

void current_sensor_feed(current_sensor_ctx_t *ctx, float raw_mv)
{
    if (ctx == NULL) return;

    // ── calibration path ──
    if (ctx->calibrating) {
        if (raw_mv >= CURRENT_SENSOR_ADC_MIN_MV && raw_mv <= CURRENT_SENSOR_ADC_MAX_MV) {
            ctx->cal_sum += raw_mv;
            ctx->cal_count++;
            if (ctx->cal_count >= ctx->cal_target) {
                ctx->calibrating = false;
                xSemaphoreGiveFromISR(ctx->cal_done_sem, NULL);
            }
        }
        return;
    }

    // ── normal path ──
    if (raw_mv < CURRENT_SENSOR_ADC_MIN_MV || raw_mv > CURRENT_SENSOR_ADC_MAX_MV) return;

    float v = s_to_real_amps(ctx, raw_mv);
    ctx->i_inst = v;
    ctx->sum_sq  += v * v;

    float v_abs = (v < 0.0f) ? -v : v;
    if (v_abs > ctx->ipeak) ctx->ipeak = v_abs;

    ctx->sample_count++;

    if (ctx->sample_count >= CURRENT_SENSOR_WINDOW_SIZE) {
        float rms_val  = sqrtf(ctx->sum_sq / (float)ctx->sample_count);
        float peak_val = ctx->ipeak;
        uint32_t n     = ctx->sample_count;

        bool plausible = (rms_val <= CURRENT_SENSOR_MAX_IRMS_A);

        if (xSemaphoreTake(ctx->mutex, 0) == pdTRUE) {
            if (plausible) {
                ctx->result.irms_a         = rms_val;
                ctx->result.ipeak_a         = peak_val;
                ctx->result.window_samples = n;
                ctx->result.valid          = true;
            } else {
                ctx->result.valid = false;
                ESP_LOGW(TAG, "Irms sanity check failed: %.2f A — sensor disconnected?", (double)rms_val);
            }
            xSemaphoreGive(ctx->mutex);
        }

        ctx->sum_sq          = 0.0f;
        ctx->ipeak          = 0.0f;
        ctx->sample_count  = 0;
    }
}

void current_sensor_get_result(const current_sensor_ctx_t *ctx, current_sensor_result_t *out)
{
    if (ctx == NULL || out == NULL) return;

    if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        *out = ctx->result;
        xSemaphoreGive(ctx->mutex);
    } else {
        memset(out, 0, sizeof(current_sensor_result_t));
        out->valid = false;
    }
}

float current_sensor_get_instantaneous(const current_sensor_ctx_t *ctx)
{
    if (ctx == NULL) return 0.0f;
    return ctx->i_inst;
}

void current_sensor_calibrate_offset(current_sensor_ctx_t *ctx, uint32_t num_samples)
{
    if (ctx == NULL || num_samples == 0) return;

    ESP_LOGI(TAG, "Calibrating DC offset over %lu samples (%.0f ms) — no signal present",
             (unsigned long)num_samples,
             (double)(num_samples * 1000.0f / 20000.0f));

    // Reset cal state, then set calibrating flag
    ctx->cal_sum      = 0.0f;
    ctx->cal_count    = 0;
    ctx->cal_target   = num_samples;
    ctx->calibrating  = true;

    // Block until feed() gives the semaphore (runs in ADC task)
    if (xSemaphoreTake(ctx->cal_done_sem, pdMS_TO_TICKS(num_samples * 2)) == pdTRUE) {
        float new_offset  = ctx->cal_sum / (float)ctx->cal_count;
        ctx->dc_offset_mv = new_offset;

        // Reset Vrms accumulator so stale data doesn't corrupt first result
        ctx->sum_sq          = 0.0f;
        ctx->ipeak          = 0.0f;
        ctx->sample_count  = 0;
        ctx->result.valid  = false;

        ESP_LOGI(TAG, "Calibration done — offset: %.1f mV (was %.1f mV, delta: %+.1f mV)",
                 (double)new_offset,
                 (double)CURRENT_SENSOR_DC_OFFSET_MV,
                 (double)(new_offset - CURRENT_SENSOR_DC_OFFSET_MV));
        ESP_LOGI(TAG, "To make permanent: #define CURRENT_SENSOR_DC_OFFSET_MV  %.1ff", (double)new_offset);
    } else {
        ctx->calibrating = false;
        ESP_LOGE(TAG, "Calibration timed out — check ADC is running");
    }
}

bool current_sensor_calibration_done(current_sensor_ctx_t *ctx)
{
    // With the semaphore approach calibrate_offset() blocks until done,
    // so this is only needed if caller wants non-blocking behaviour.
    // Returns true when not calibrating and a valid offset has been set.
    if (ctx == NULL) return false;
    return !ctx->calibrating;
}

void current_sensor_reset(current_sensor_ctx_t *ctx)
{
    if (ctx == NULL) return;
    if (xSemaphoreTake(ctx->mutex, portMAX_DELAY) == pdTRUE) {
        ctx->sum_sq          = 0.0f;
        ctx->ipeak          = 0.0f;
        ctx->sample_count  = 0;
        ctx->i_inst        = 0.0f;
        ctx->result.valid  = false;
        xSemaphoreGive(ctx->mutex);
    }
}

void current_sensor_deinit(current_sensor_ctx_t *ctx)
{
    if (ctx == NULL) return;
    vSemaphoreDelete(ctx->mutex);
    vSemaphoreDelete(ctx->cal_done_sem);
    free(ctx);
}