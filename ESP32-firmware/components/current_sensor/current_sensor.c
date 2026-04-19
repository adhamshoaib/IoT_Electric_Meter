#include "current_sensor.h"

#include <math.h>

float current_sensor_convert(int16_t mv)
{
    return ((float)mv - CURRENT_SENSOR_BIAS_MV) * CURRENT_SENSOR_SCALE_FACTOR;
}

float current_sensor_rms(const int16_t *mv_samples, size_t count)
{
    if (mv_samples == NULL || count == 0U)
    {
        return 0.0f;
    }

    float mean_i = 0.0f;
    for (size_t i = 0; i < count; ++i)
    {
        mean_i += current_sensor_convert(mv_samples[i]);
    }
    mean_i /= (float)count;

    float sum_sq = 0.0f;
    for (size_t i = 0; i < count; ++i)
    {
        const float i_ac = current_sensor_convert(mv_samples[i]) - mean_i;
        sum_sq += i_ac * i_ac;
    }

    return sqrtf(sum_sq / (float)count);
}
