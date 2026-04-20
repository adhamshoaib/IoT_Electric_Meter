#include "energy_calc.h"

#include <stdbool.h>
#include <math.h>

#define ENERGY_CALC_MIN_APPARENT_POWER_VA 1e-6f
#define WATT_SECONDS_PER_KWH 3600000.0f

static float lerp(float a, float b, float t)
{
    return a + ((b - a) * t);
}

static bool interpolate_shifted_sample(const float *samples,
                                       size_t count,
                                       size_t index,
                                       float shift_samples,
                                       float *out)
{
    const float shifted_index = (float)index + shift_samples;
    if (shifted_index < 0.0f || shifted_index > (float)(count - 1U))
    {
        return false;
    }

    const size_t i0 = (size_t)floorf(shifted_index);
    const size_t i1 = (i0 + 1U < count) ? (i0 + 1U) : i0;
    const float frac = shifted_index - (float)i0;
    *out = lerp(samples[i0], samples[i1], frac);

    return true;
}

static float clamp_0_1(float x)
{
    if (x < 0.0f)
    {
        return 0.0f;
    }
    if (x > 1.0f)
    {
        return 1.0f;
    }
    return x;
}

esp_err_t energy_calc_power(power_calc_result_t *out,
                            const float *v_samples,
                            const float *i_samples,
                            size_t count,
                            float current_advance_samples,
                            float voltage_advance_samples)
{
    if (out == NULL || v_samples == NULL || i_samples == NULL || count < 2U ||
        !isfinite(current_advance_samples) || !isfinite(voltage_advance_samples))
    {
        if (out != NULL)
        {
            *out = (power_calc_result_t){0};
        }
        return ESP_ERR_INVALID_ARG;
    }

    *out = (power_calc_result_t){0};

    size_t valid = 0U;

    // Mean values for real-power path (after phase shift/interpolation).
    float mean_v = 0.0f;
    float mean_i = 0.0f;
    for (size_t k = 0; k < count; ++k)
    {
        float v_shifted = 0.0f;
        float i_shifted = 0.0f;
        const bool have_v = interpolate_shifted_sample(v_samples,
                                                       count,
                                                       k,
                                                       voltage_advance_samples,
                                                       &v_shifted);
        const bool have_i = interpolate_shifted_sample(i_samples,
                                                       count,
                                                       k,
                                                       current_advance_samples,
                                                       &i_shifted);

        if (!have_v || !have_i)
        {
            continue;
        }

        mean_v += v_shifted;
        mean_i += i_shifted;
        ++valid;
    }

    if (valid == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }

    mean_v /= (float)valid;
    mean_i /= (float)valid;

    float sum_p = 0.0f;
    for (size_t k = 0; k < count; ++k)
    {
        float v_shifted = 0.0f;
        float i_shifted = 0.0f;
        const bool have_v = interpolate_shifted_sample(v_samples,
                                                       count,
                                                       k,
                                                       voltage_advance_samples,
                                                       &v_shifted);
        const bool have_i = interpolate_shifted_sample(i_samples,
                                                       count,
                                                       k,
                                                       current_advance_samples,
                                                       &i_shifted);

        if (!have_v || !have_i)
        {
            continue;
        }

        const float v_ac = v_shifted - mean_v;
        const float i_ac = i_shifted - mean_i;

        sum_p += v_ac * i_ac;
    }

    // RMS path for apparent power uses unshifted samples so phase compensation
    // does not change V_rms/I_rms magnitude and does not distort apparent power.
    float mean_v_rms = 0.0f;
    float mean_i_rms = 0.0f;
    for (size_t k = 0; k < count; ++k)
    {
        mean_v_rms += v_samples[k];
        mean_i_rms += i_samples[k];
    }
    mean_v_rms /= (float)count;
    mean_i_rms /= (float)count;

    float sum_v_sq = 0.0f;
    float sum_i_sq = 0.0f;
    for (size_t k = 0; k < count; ++k)
    {
        const float v_ac = v_samples[k] - mean_v_rms;
        const float i_ac = i_samples[k] - mean_i_rms;
        sum_v_sq += v_ac * v_ac;
        sum_i_sq += i_ac * i_ac;
    }

    const float inv_n_real = 1.0f / (float)valid;
    const float inv_n_rms = 1.0f / (float)count;
    float real_power_w = sum_p * inv_n_real;
    const float vrms = sqrtf(sum_v_sq * inv_n_rms);
    const float irms = sqrtf(sum_i_sq * inv_n_rms);
    const float apparent_power_va = vrms * irms;

    if (real_power_w < 0.0f)
    {
        real_power_w = 0.0f;
    }

    float power_factor = 0.0f;
    if (apparent_power_va >= ENERGY_CALC_MIN_APPARENT_POWER_VA)
    {
        power_factor = clamp_0_1(real_power_w / apparent_power_va);
    }

    out->real_power_w = real_power_w;
    out->apparent_power_va = apparent_power_va;
    out->power_factor = power_factor;

    return ESP_OK;
}

esp_err_t energy_calc_accumulate_kwh(float *accumulator_kwh,
                                     float real_power_w,
                                     float window_duration_s)
{
    if (accumulator_kwh == NULL || window_duration_s <= 0.0f)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (real_power_w < 0.0f)
    {
        real_power_w = 0.0f;
    }

    const float delta_kwh = (real_power_w * window_duration_s) / WATT_SECONDS_PER_KWH;
    *accumulator_kwh += delta_kwh;

    return ESP_OK;
}
