#include "energy_metering.h"

typedef struct {
    energy_metering_calibration_t cal;
    float total_energy_kwh;
    float energy_divisor;
    int32_t prev_cfa_cnt;
    int32_t prev_cfb_cnt;
    bool counter_valid;
    bool initialized;
} energy_metering_t;

static energy_metering_t s_ctx;

static bool calibration_is_valid(const energy_metering_calibration_t *cal)
{
    return (cal != NULL) &&
           (cal->vrms_scale > 0.0f) &&
           (cal->vref > 0.0f) &&
           (cal->divider_ratio > 0.0f) &&
           (cal->vac_fine_gain > 0.0f) &&
           (cal->irms_scale > 0.0f) &&
           (cal->ia_pin_fine_gain > 0.0f) &&
           (cal->ia_cal_a_per_mv > 0.0f) &&
           (cal->energy_ref > 0.0f) &&
           (cal->cf_count_scale > 0.0f) &&
           (cal->vac_noise_floor_v >= 0.0f) &&
           (cal->ia_noise_floor_a >= 0.0f) &&
           (cal->vp_noise_floor_mv >= 0.0f);
}

static uint32_t vrms_raw_apply_zero_offset(uint32_t v_rms_raw)
{
    if (v_rms_raw <= s_ctx.cal.vrms_zero_offset)
    {
        return 0U;
    }

    return v_rms_raw - s_ctx.cal.vrms_zero_offset;
}

static float vrms_raw_to_mains_v(uint32_t v_rms_raw)
{
    const uint32_t corrected_raw = vrms_raw_apply_zero_offset(v_rms_raw);
    const float vp_mv = ((float)corrected_raw * s_ctx.cal.vref) / s_ctx.cal.vrms_scale;
    const float mains_v = ((vp_mv * s_ctx.cal.divider_ratio) / 1000.0f) * s_ctx.cal.vac_fine_gain;

    return (mains_v < s_ctx.cal.vac_noise_floor_v) ? 0.0f : mains_v;
}

static float irms_raw_to_ip_mv(uint32_t i_rms_raw)
{
    const float ip_mv = (((float)i_rms_raw * s_ctx.cal.vref) / s_ctx.cal.irms_scale) *
                        s_ctx.cal.ia_pin_fine_gain;
    return (ip_mv < s_ctx.cal.vp_noise_floor_mv) ? 0.0f : ip_mv;
}

static float irms_raw_to_amps(uint32_t i_rms_raw)
{
    const float ip_mv = irms_raw_to_ip_mv(i_rms_raw);
    const float amps = ip_mv * s_ctx.cal.ia_cal_a_per_mv;

    return (amps < s_ctx.cal.ia_noise_floor_a) ? 0.0f : amps;
}

static float frame_energy_kwh(const bl0939_raw_data_t *raw)
{
    if (raw == NULL)
    {
        return 0.0f;
    }

    if (!s_ctx.counter_valid)
    {
        s_ctx.prev_cfa_cnt = raw->cfa_cnt;
        s_ctx.prev_cfb_cnt = raw->cfb_cnt;
        s_ctx.counter_valid = true;
        return 0.0f;
    }

    const int32_t delta_cfa = raw->cfa_cnt - s_ctx.prev_cfa_cnt;
    const int32_t delta_cfb = raw->cfb_cnt - s_ctx.prev_cfb_cnt;

    s_ctx.prev_cfa_cnt = raw->cfa_cnt;
    s_ctx.prev_cfb_cnt = raw->cfb_cnt;

    const int32_t delta_total = delta_cfa + delta_cfb;
    if (delta_total <= 0)
    {
        return 0.0f;
    }

    return (float)delta_total / s_ctx.energy_divisor;
}

esp_err_t energy_metering_init(const energy_metering_config_t *config)
{
    if (config == NULL || !calibration_is_valid(&config->calibration))
    {
        return ESP_ERR_INVALID_ARG;
    }

    s_ctx.cal = config->calibration;
    s_ctx.energy_divisor = s_ctx.cal.energy_ref * s_ctx.cal.cf_count_scale;
    s_ctx.total_energy_kwh = 0.0f;
    s_ctx.prev_cfa_cnt = 0;
    s_ctx.prev_cfb_cnt = 0;
    s_ctx.counter_valid = false;
    s_ctx.initialized = true;

    return ESP_OK;
}

esp_err_t energy_metering_read(energy_metering_data_t *out, uint32_t timeout_ms)
{
    if (!s_ctx.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    bl0939_raw_data_t raw;
    esp_err_t ret = bl0939_read_raw(&raw, timeout_ms);
    if (ret != ESP_OK)
    {
        return ret;
    }

    s_ctx.total_energy_kwh += frame_energy_kwh(&raw);

    energy_metering_data_t data;
    data.voltage_v = vrms_raw_to_mains_v(raw.v_rms);
    data.current_a = irms_raw_to_amps(raw.ia_rms);
    data.total_energy_kwh = s_ctx.total_energy_kwh;

    *out = data;
    return ESP_OK;
}

void energy_metering_reset_energy(void)
{
    s_ctx.total_energy_kwh = 0.0f;
    s_ctx.prev_cfa_cnt = 0;
    s_ctx.prev_cfb_cnt = 0;
    s_ctx.counter_valid = false;
}
