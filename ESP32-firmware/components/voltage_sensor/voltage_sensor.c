#include "voltage_sensor.h"

#include <math.h>

float voltage_sensor_convert(int16_t mv)
{
    return ((float)mv - VOLTAGE_SENSOR_BIAS_MV) * VOLTAGE_SENSOR_SCALE_FACTOR;
}

float voltage_sensor_rms(const int16_t *mv_samples, size_t count)
{
    if (mv_samples == NULL || count == 0U)
    {
        return 0.0f;
    }

    float mean_v = 0.0f;
    for (size_t i = 0; i < count; ++i)
    {
        mean_v += voltage_sensor_convert(mv_samples[i]);
    }
    mean_v /= (float)count;

    float sum_sq = 0.0f;
    for (size_t i = 0; i < count; ++i)
    {
        const float v_ac = voltage_sensor_convert(mv_samples[i]) - mean_v;
        sum_sq += v_ac * v_ac;
    }

    return sqrtf(sum_sq / (float)count);
}
