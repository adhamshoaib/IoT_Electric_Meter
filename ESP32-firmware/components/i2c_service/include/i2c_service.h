/**
 * @file i2c_service.h
 * @brief Singleton I2C bus wrapper with mutex-protected access.
 *
 * Owns the I2C master bus and provides raw write / write-read primitives.
 * Device-specific register maps, configuration sequences, and data parsing
 * belong in driver layers above this service.
 *
 * All APIs are task-safe and serialize bus access via an internal mutex.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "hal/i2c_types.h"
/* ------------------------------------------------------------------ */
/*  Lifecycle                                                         */
/* ------------------------------------------------------------------ */
/**
 * @brief Initialize the I2C bus and create the bus mutex.
 *
 * Must be called once before any other i2c_service function.
 *
 * @param port   I2C port number (for example, I2C_NUM_0).
 * @param sda    GPIO pin number for SDA.
 * @param scl    GPIO pin number for SCL.
 * @param clk_hz Bus clock in Hz (for example, 400000 for Fast Mode).
 *
 * @return ESP_OK on success.
 * @return ESP_ERR_INVALID_STATE if already initialized.
 * @return ESP_ERR_INVALID_ARG if one or more arguments are invalid.
 * @return Other esp_err_t codes from the ESP-IDF I2C driver.
 */
esp_err_t i2c_service_init(i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t clk_hz);
/**
 * @brief Deinitialize the I2C bus and release all service resources.
 *
 * @return ESP_OK on success.
 * @return ESP_ERR_INVALID_STATE if the service is not initialized.
 * @return Other esp_err_t codes from the ESP-IDF I2C driver.
 */
esp_err_t i2c_service_deinit(void);
/* ------------------------------------------------------------------ */
/*  Raw bus access                                                    */
/* ------------------------------------------------------------------ */
/**
 * @brief Write bytes to an I2C device.
 *
 * Acquires the service mutex, performs the transaction, then releases it.
 *
 * @param dev_addr 7-bit I2C device address.
 * @param data     Pointer to bytes to transmit.
 * @param len      Number of bytes to transmit.
 *
 * @return ESP_OK on success.
 * @return ESP_ERR_INVALID_STATE if the service is not initialized.
 * @return ESP_ERR_INVALID_ARG if data is NULL or len is 0.
 * @return ESP_ERR_TIMEOUT if mutex acquisition timed out.
 * @return Other esp_err_t I2C transfer errors.
 */
esp_err_t i2c_service_write(uint8_t dev_addr, const uint8_t *data, size_t len);
/**
 * @brief Write one register address, then read bytes back.
 *
 * Uses one mutex hold for the entire write-read sequence.
 *
 * @param dev_addr 7-bit I2C device address.
 * @param reg_addr Register address to write before reading.
 * @param out      Output buffer for received data.
 * @param len      Number of bytes to read into @p out.
 *
 * @return ESP_OK on success.
 * @return ESP_ERR_INVALID_STATE if the service is not initialized.
 * @return ESP_ERR_INVALID_ARG if out is NULL or len is 0.
 * @return ESP_ERR_TIMEOUT if mutex acquisition timed out.
 * @return Other esp_err_t I2C transfer errors.
 */
esp_err_t i2c_service_write_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *out, size_t len);
/* ------------------------------------------------------------------ */
/*  Bus handle access for driver registration                         */
/* ------------------------------------------------------------------ */
/**
 * @brief Get the raw bus handle for device registration.
 *
 * Device drivers can use this in their init sequence with
 * i2c_master_bus_add_device().
 *
 * @return Bus handle when initialized, otherwise NULL.
 */
i2c_master_bus_handle_t i2c_service_get_bus_handle(void);