#include "adc_driver.h"

#include <stdbool.h>
#include "esp_check.h"
#include "esp_adc/adc_continuous.h"

#define ADC_SAMPLE_RATE_HZ 20000 // 400 samples/cycle at 50Hz
#define CYCLES_PER_WINDOW 4      // compute power every 4 cycles (80ms)
#define SAMPLES_PER_WINDOW (ADC_SAMPLE_RATE_HZ / 50 * CYCLES_PER_WINDOW)
#define ADC_FRAME_SIZE (SAMPLES_PER_WINDOW * sizeof(adc_digi_output_data_t))
#define ADC_RING_BUF_SIZE (ADC_FRAME_SIZE * 2)

typedef struct
{
    bool init;
    bool running;
} adc_driver_t;

#define NUM_CHANNELS 2

static const adc_channel_t channels[NUM_CHANNELS] = {ADC_CHANNEL_6, ADC_CHANNEL_7};

static adc_driver_t adc_driver;
static adc_continuous_handle_t adc_handle;

static const char *TAG = "ADC_DRIVER";

esp_err_t adc_driver_init(void)
{
    if (adc_driver.init)
        return ESP_ERR_INVALID_STATE;

    adc_continuous_handle_cfg_t handle_config = {
        .max_store_buf_size = ADC_RING_BUF_SIZE,
        .conv_frame_size = ADC_FRAME_SIZE,
    };

    ESP_RETURN_ON_ERROR(adc_continuous_new_handle(&handle_config, &adc_handle), TAG, "adc_continuous_new_handle failed");

    adc_continuous_config_t adc_config = {
        .pattern_num = NUM_CHANNELS,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
        .sample_freq_hz = ADC_SAMPLE_RATE_HZ,
    };

    adc_digi_pattern_config_t channel_config[2];
    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        channel_config[i].channel = channels[i];
        channel_config[i].atten = ADC_ATTEN_DB_12;
        channel_config[i].bit_width = ADC_BITWIDTH_12;
        channel_config[i].unit = ADC_UNIT_1;
    }
    adc_config.adc_pattern = channel_config;

    esp_err_t ret = adc_continuous_config(adc_handle, &adc_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "adc_continuous_config failed: %s", esp_err_to_name(ret));
        goto cleanup_handle;
    }

    adc_driver.init = true;
    adc_driver.running = false;

    return ESP_OK;

cleanup_handle:
    if (adc_continuous_deinit(adc_handle) != ESP_OK)
        ESP_LOGE(TAG, "adc_continuous_deinit failed during cleanup");
    return ret;
}

esp_err_t adc_driver_deinit(void)
{
    if (!adc_driver.init)
        return ESP_ERR_INVALID_STATE;

    if (adc_driver.running)
        ESP_RETURN_ON_ERROR(adc_driver_stop(), TAG, "adc_driver_stop failed");

    ESP_RETURN_ON_ERROR(adc_continuous_deinit(adc_handle), TAG, "adc_continuous_deinit failed");

    adc_handle = NULL;
    adc_driver.init = false;

    return ESP_OK;
}

esp_err_t adc_driver_start(void)
{
    if (!adc_driver.init || adc_driver.running)
        return ESP_ERR_INVALID_STATE;

    ESP_RETURN_ON_ERROR(adc_continuous_start(adc_handle), TAG, "adc_continuous_start failed");

    adc_driver.running = true;

    return ESP_OK;
}

esp_err_t adc_driver_stop(void)
{
    if (!adc_driver.init || !adc_driver.running)
        return ESP_ERR_INVALID_STATE;

    ESP_RETURN_ON_ERROR(adc_continuous_stop(adc_handle), TAG, "adc_continuous_stop failed");

    adc_driver.running = false;

    return ESP_OK;
}