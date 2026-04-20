#include "voltage_sensor.h"

#include <math.h>

float voltage_sensor_convert(int16_t raw)
{
    return ((float)raw - VOLTAGE_SENSOR_BIAS_RAW) * VOLTAGE_SENSOR_RAW_TO_VOLTS;
}

float voltage_sensor_rms(const adc_sample_pair_t *pairs, size_t count)
{
    if (pairs == NULL || count == 0U)
    {
        return 0.0f;
    }

    // Two-pass RMS:
    // 1) Compute mean after known bias removal to capture residual DC offset.
    // 2) Compute AC RMS around that mean.
    // We intentionally call voltage_sensor_convert() in both passes instead of
    // caching a temporary float buffer to keep memory usage minimal.
    // Trade-off: this doubles conversion CPU cost per sample.
    // If conversion becomes non-trivial (e.g., LUT/nonlinear correction),
    // re-evaluate this choice and consider caching converted samples.
    float mean_v = 0.0f;
    for (size_t i = 0; i < count; ++i)
    {
        mean_v += voltage_sensor_convert(pairs[i].v_raw);
    }
    mean_v /= (float)count;

    float sum_sq = 0.0f;
    for (size_t i = 0; i < count; ++i)
    {
        const float v_ac = voltage_sensor_convert(pairs[i].v_raw) - mean_v;
        sum_sq += v_ac * v_ac;
    }

    return sqrtf(sum_sq / (float)count);
}
