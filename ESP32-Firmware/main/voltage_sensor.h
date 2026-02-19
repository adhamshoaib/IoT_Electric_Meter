#ifndef VOLTAGE_SENSOR_H
#define VOLTAGE_SENSOR_H

#include "adc_wrapper.h"
#include "esp_err.h"

#define VOLTAGE_ATTEN_DB ADC_ATTEN_DB_11
#define VOLTAGE_DC_OFFSET 2500

#define MAINS_FREQUENCY 50.0f
#define MAINS_PERIOD_MS (1000.0f / MAINS_FREQUENCY)
#define RMS_SAMPLE_COUNT 200  
#define RMS_TOTAL_SAMPLES (RMS_CYCLES * RMS_SAMPLE_COUNT)   
#define SAMPLE_DELAY_US ((uint32_t)((1000000.0f / MAINS_FREQUENCY) * RMS_CYCLES / RMS_TOTAL_SAMPLES))
 

#define VOLTAGE_SCALE_FACTOR 341.5f
#define RMS_CYCLES 10  


typedef struct {
    adc_wrapper_t *adc;
    adc_channel_t channel;
} voltage_sensor_t;

esp_err_t voltage_sensor_init(voltage_sensor_t *sensor);

esp_err_t voltage_sensor_read_rms(voltage_sensor_t *sensor, float *out_mv);

//esp_err_t voltage_sensor_deinit(voltage_sensor_t *sensor);

#endif