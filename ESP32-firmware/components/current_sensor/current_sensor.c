#include "current_sensor.h"

#include <math.h>

float current_sensor_convert(int16_t raw)
{
    return ((float)raw - CURRENT_SENSOR_BIAS_RAW) * CURRENT_SENSOR_RAW_TO_AMPS;
}

float current_sensor_rms(const int16_t *raw_samples, size_t count)
{
    if (raw_samples == NULL || count == 0U)
    {
        return 0.0f;
    }

    // Two-pass RMS:
    // 1) Compute mean after known bias removal to capture residual DC offset.
    // 2) Compute AC RMS around that mean.
    // We intentionally call current_sensor_convert() in both passes instead of
    // caching a temporary float buffer to keep memory usage minimal.
    // Trade-off: this doubles conversion CPU cost per sample.
    // If conversion becomes non-trivial (e.g., LUT/nonlinear correction),
    // consider caching converted samples.
    float mean_i = 0.0f;
    for (size_t i = 0; i < count; ++i)
    {
        mean_i += current_sensor_convert(raw_samples[i]);
    }
    mean_i /= (float)count;

    float sum_sq = 0.0f;
    for (size_t i = 0; i < count; ++i)
    {
        const float i_ac = current_sensor_convert(raw_samples[i]) - mean_i;
        sum_sq += i_ac * i_ac;
    }

    return sqrtf(sum_sq / (float)count);
}
