#include "adc_wrapper.h"

static esp_err_t adc_wrapper_calibration_init(adc_wrapper_t *adc, adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_bitwidth_t bitwidth)
{
    if (!adc || channel >= ADC_MAX_CHANNELS) return ESP_ERR_INVALID_ARG;

    adc_cali_line_fitting_config_t cfg = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = bitwidth,
    };

    esp_err_t ret = adc_cali_create_scheme_line_fitting(&cfg, &adc->cali[channel]);
    adc->cali_ready[channel] = (ret == ESP_OK);
    return ret;
}

esp_err_t adc_wrapper_init(adc_wrapper_t *adc, adc_unit_t unit)
{
    if (!adc) return ESP_ERR_INVALID_ARG;
    if (adc->initialized) return ESP_OK;   // prevent double init

    adc_oneshot_unit_init_cfg_t cfg = {
        .unit_id = unit,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    esp_err_t ret = adc_oneshot_new_unit(&cfg, &adc->handle);
    if (ret != ESP_OK) {
        adc->handle = NULL;
        return ret;
    }

    adc->unit = unit;
    adc->initialized = true;
    return ESP_OK;
}

esp_err_t adc_wrapper_config_channel(adc_wrapper_t *adc, adc_channel_t channel, adc_atten_t atten)
{
    if (!adc || !adc->initialized) return ESP_ERR_INVALID_STATE;

    adc_oneshot_chan_cfg_t cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = atten, 
    };

    esp_err_t ret = adc_oneshot_config_channel(adc->handle, channel, &cfg);
    if (ret != ESP_OK) return ret;

    if (!adc->cali_ready[channel]) {
    ret = adc_wrapper_calibration_init(adc, adc->unit, channel, atten, ADC_BITWIDTH_DEFAULT);
    if (ret != ESP_OK) return ret;
    adc->cali_ready[channel] = true;
}

    if (channel < ADC_MAX_CHANNELS) {
        adc->configured[channel] = true;  // mark this channel as configured
    }

    return ESP_OK;
}

esp_err_t adc_wrapper_read_mv(adc_wrapper_t *adc, adc_channel_t channel, int *out_mv)
{
    if (!adc || !adc->handle || !out_mv) {
        printf("Invalid argument to adc_wrapper_read_mv\n");
        return ESP_ERR_INVALID_ARG;
    }
    if (!adc->configured[channel]) {
        printf("ADC channel %d not configured\n", channel);
        return ESP_ERR_INVALID_STATE;
    }
    int raw;
    esp_err_t ret = adc_oneshot_read(adc->handle, channel, &raw);
    if (ret != ESP_OK)
        return ret;

    if (!adc->cali_ready[channel]) {
        printf("ADC channel %d calibration not ready\n", channel);
        return ESP_ERR_INVALID_STATE;
    }
    return adc_cali_raw_to_voltage(adc->cali[channel], raw, out_mv);
}

esp_err_t adc_wrapper_deinit_channel(adc_wrapper_t *adc, adc_channel_t channel)
{
    if (!adc || channel >= ADC_MAX_CHANNELS) return ESP_ERR_INVALID_ARG;

    if (adc->cali_ready[channel]) {
        adc_cali_delete_scheme_line_fitting(adc->cali[channel]);
        adc->cali_ready[channel] = false;
    }

    adc->configured[channel] = false;

    return ESP_OK;
}

esp_err_t adc_wrapper_deinit(adc_wrapper_t *adc)
{
    if (!adc) return ESP_ERR_INVALID_ARG;

    for (int ch = 0; ch < ADC_MAX_CHANNELS; ch++) {
        if (adc->cali_ready[ch]) {
            adc_cali_delete_scheme_line_fitting(adc->cali[ch]);
            adc->cali_ready[ch] = false;
        }
        adc->configured[ch] = false;
    }

    if (adc->handle) {
        adc_oneshot_del_unit(adc->handle);
        adc->handle = NULL;
    }

    adc->initialized = false;
    return ESP_OK;
}



