/**
 * @file adc_driver.h
 * @brief ADC continuous mode driver for simultaneous voltage and current sampling.
 *
 * Wraps the ESP-IDF ADC continuous driver to deliver synchronized voltage and
 * current sample pairs via DMA. Both channels are sampled in a single sweep,
 * guaranteeing temporal alignment required for real power calculation.
 *
 * This driver is a singleton — only one instance exists per device.
 * All ESP-IDF ADC handles are internal implementation details and are not
 * exposed in this API.
 *
 * Typical usage:
 * @code
 *   adc_driver_init();
 *   adc_driver_start();
 *
 *   adc_sample_pair_t buf[SAMPLES_PER_WINDOW];
 *   size_t count = 0;
 *   adc_driver_read(buf, SAMPLES_PER_WINDOW, &count, 100);
 *
 *   adc_driver_stop();
 *   adc_driver_deinit();
 * @endcode
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/* ------------------------------------------------------------------ */
/*  Types                                                               */
/* ------------------------------------------------------------------ */

/**
 * @brief A single synchronized voltage + current sample pair.
 *
 * Both values are raw ADC counts from the same DMA sweep, guaranteeing
 * they represent the same instant in time.
 */
typedef struct
{
    uint16_t v_raw; /**< Voltage channel raw ADC count. */
    uint16_t i_raw; /**< Current channel raw ADC count. */
    int32_t v_mv;
    int32_t i_mv;
} adc_sample_pair_t;

/* ------------------------------------------------------------------ */
/*  Lifecycle                                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief Initialize the ADC continuous driver.
 *
 * Configures both the voltage and current ADC channels, allocates the
 * internal DMA buffer, and prepares the driver for sampling. Does not
 * begin sampling — call adc_driver_start() to begin.
 *
 * @return ESP_OK on success.
 * @return ESP_ERR_INVALID_STATE if the driver is already initialized.
 * @return ESP_ERR_NO_MEM if DMA buffer allocation fails.
 * @return Other esp_err_t codes propagated from the ESP-IDF ADC driver.
 */
esp_err_t adc_driver_init(void);

/**
 * @brief Deinitialize the ADC continuous driver and free all resources.
 *
 * Stops sampling if currently running, then releases the DMA buffer
 * and ADC handle. Safe to call after adc_driver_stop() or directly
 * if the driver was never started.
 *
 * @return ESP_OK on success.
 * @return ESP_ERR_INVALID_STATE if the driver was not initialized.
 */
esp_err_t adc_driver_deinit(void);

/* ------------------------------------------------------------------ */
/*  Control                                                             */
/* ------------------------------------------------------------------ */

/**
 * @brief Start continuous ADC sampling on both channels.
 *
 * Must be called after adc_driver_init(). After this call, the DMA
 * buffer begins filling with interleaved voltage and current samples.
 *
 * @return ESP_OK on success.
 * @return ESP_ERR_INVALID_STATE if the driver is not initialized or
 *         is already running.
 */
esp_err_t adc_driver_start(void);

/**
 * @brief Stop continuous ADC sampling.
 *
 * Halts the DMA sweep. The driver remains initialized and can be
 * restarted with adc_driver_start().
 *
 * @return ESP_OK on success.
 * @return ESP_ERR_INVALID_STATE if the driver is not currently running.
 */
esp_err_t adc_driver_stop(void);

/* ------------------------------------------------------------------ */
/*  Data retrieval                                                      */
/* ------------------------------------------------------------------ */

/**
 * @brief Read synchronized sample pairs from the DMA buffer.
 *
 * Blocks until @p max_count pairs are available or @p timeout_ms
 * elapses. Parses raw DMA frames into adc_sample_pair_t structs —
 * the caller receives clean {v_raw, i_raw} pairs with no knowledge
 * of the underlying DMA frame format.
 *
 * A timeout of 0 is valid and makes the call non-blocking.
 *
 * @param[out] out        Buffer to write sample pairs into.
 * @param[in]  max_count  Maximum number of pairs to read.
 * @param[out] out_count  Actual number of pairs written. Must not be NULL.
 * @param[in]  timeout_ms Timeout in milliseconds. 0 = non-blocking.
 *
 * @return ESP_OK if at least one pair was read.
 * @return ESP_ERR_TIMEOUT if no data was available within the timeout.
 * @return ESP_ERR_INVALID_STATE if the driver is not running.
 * @return ESP_ERR_INVALID_ARG if out or out_count is NULL.
 */
esp_err_t adc_driver_read(adc_sample_pair_t *out, size_t max_count, size_t *out_count, uint32_t timeout_ms);
