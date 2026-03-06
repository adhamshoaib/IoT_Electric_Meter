#ifndef ADC_CONT.H
#define ADC_CONT.H

#include "esp_err.h"
#include "esp_adc/adc_continuous.h"
#include "hal/adc_types.h"
#include <stdint.h>
#include <stdbool.h>

// ─── tunables ────────────────────────────────────────────────────────────────
#define ADC_DRV_SAMPLE_FREQ_HZ   20000
#define ADC_DRV_FRAME_SIZE       256          // bytes per DMA frame
#define ADC_DRV_BUF_SIZE         2048         // internal DMA store buffer (bytes)
#define ADC_DRV_MAX_SAMPLES      (ADC_DRV_FRAME_SIZE / SOC_ADC_DIGI_RESULT_BYTES)
#define ADC_DRV_TASK_STACK       8192
#define ADC_DRV_TASK_PRIORITY    (configMAX_PRIORITIES - 2)

// ─── public sample type ──────────────────────────────────────────────────────

/**
 * @brief One calibrated sample delivered to the user callback.
 *
 * `voltage_mv` is already converted through the line-fitting calibration
 * scheme (or the raw ADC counts cast to float when calibration is
 * unavailable).  The raw value is intentionally NOT exposed — callers should
 * never need it; the sensor layer works exclusively in mV.
 */
typedef struct {
    adc_channel_t channel;      ///< originating ADC channel
    float         voltage_mv;   ///< calibrated voltage in millivolts
} adc_drv_sample_t;

// ─── callback ────────────────────────────────────────────────────────────────

/**
 * @brief User callback invoked from the ADC task (NOT from ISR context).
 *
 * @param samples   Pointer to an array of parsed, calibrated samples.
 *                  Valid only for the duration of the callback.
 * @param count     Number of valid entries in @p samples.
 */
typedef void (*adc_drv_cb_t)(const adc_drv_sample_t *samples, uint32_t count);

// ─── API ─────────────────────────────────────────────────────────────────────

/**
 * @brief Initialise the ADC continuous driver.
 *
 * Creates the internal FreeRTOS task, configures the ADC continuous handle,
 * registers the conversion-done ISR, and sets up line-fitting calibration if
 * supported by the target.
 *
 * @param channels      Array of ADC channels to sample (ADC_UNIT_1 only).
 * @param num_channels  Length of @p channels (1–SOC_ADC_PATT_LEN_MAX).
 * @param callback      Function called with each batch of samples, or NULL.
 *
 * @return ESP_OK on success, or an esp_err_t error code.
 */
esp_err_t adc_drv_init(const adc_channel_t *channels,
                       uint8_t              num_channels,
                       adc_drv_cb_t         callback);

/**
 * @brief Start continuous ADC conversions.
 * @return ESP_OK, or ESP_ERR_INVALID_STATE if not initialised.
 */
esp_err_t adc_drv_start(void);

/**
 * @brief Stop continuous ADC conversions.
 * @return ESP_OK, or ESP_ERR_INVALID_STATE if not initialised.
 */
esp_err_t adc_drv_stop(void);

/**
 * @brief Stop, deinitialise, and free all driver resources.
 * @return ESP_OK, or ESP_ERR_INVALID_STATE if not initialised.
 */
esp_err_t adc_drv_deinit(void);

#endif