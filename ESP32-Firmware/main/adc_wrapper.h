#ifndef ADC_WRAPPER_H
#define ADC_WRAPPER_H

#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#define v_ref 1100
#define ADC_MAX_CHANNELS 8
#define ADC_ATTEN_COUNT 2


typedef struct {
    adc_oneshot_unit_handle_t handle;
    adc_unit_t unit;
    bool initialized;

    adc_cali_handle_t cali[ADC_MAX_CHANNELS];
    bool cali_ready[ADC_MAX_CHANNELS];
    bool configured[ADC_MAX_CHANNELS];
} adc_wrapper_t;


    
esp_err_t adc_wrapper_init(adc_wrapper_t *adc, adc_unit_t unit);

esp_err_t adc_wrapper_config_channel(adc_wrapper_t *adc, adc_channel_t channel, adc_atten_t atten);

esp_err_t adc_wrapper_read_mv(adc_wrapper_t *adc, adc_channel_t channel, int *out_mv);

esp_err_t adc_wrapper_deinit(adc_wrapper_t *adc);



#endif
