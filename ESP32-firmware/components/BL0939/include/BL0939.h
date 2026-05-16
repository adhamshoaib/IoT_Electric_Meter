/**
 * @file BL0939.h
 * @brief Singleton UART driver for the BL0939 dual-channel energy metering IC.
 *
 * ### Design overview
 * This driver is intentionally a singleton: the BL0939 occupies one UART bus
 * and most embedded targets run exactly one instance. The caller owns the
 * uart_service handle and must keep it valid for the lifetime of this driver.
 *
 * ### Typical usage
 * @code
 * bl0939_config_t cfg = {
 *     .uart                  = my_uart_handle,
 *     .calibration           = { .voltage_ref = 15200.0f,
 *                                .current_ref = 324004.0f,
 *                                .energy_ref  = 3304.0f },
 *     .current_channel       = BL0939_CURRENT_CHANNEL_SUM,
 *     .default_timeout_ms    = 200,
 *     .auto_request_before_read = true,
 * };
 * ESP_ERROR_CHECK(bl0939_init(&cfg));
 *
 * bl0939_measurements_t m;
 * if (bl0939_read_measurements(&m, BL0939_TIMEOUT_USE_DEFAULT) == ESP_OK) {
 *     ESP_LOGI(TAG, "%.1f V  %.3f A  %.4f kWh", m.voltage_v, m.current_a, m.energy_kwh);
 * }
 * @endcode
 *
 * ### Thread safety
 * This driver is **not** thread-safe. If multiple tasks share the driver,
 * the caller must guard every API call with a mutex. No internal locking
 * is performed.
 *
 * ### Frame validation
 * Every received frame is validated against the BL0939 header byte
 * (0x55) and its trailing 8-bit checksum. Frames that fail either check
 * are rejected with ESP_ERR_INVALID_RESPONSE.
 */

#ifndef BL0939_H
#define BL0939_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "uart_service.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* -------------------------------------------------------------------------
 * Constants
 * ---------------------------------------------------------------------- */

/** Total byte length of one BL0939 data frame (header + payload + checksum). */
#define BL0939_FRAME_SIZE_BYTES 35U

/** Expected value of the first byte of every BL0939 frame. */
#define BL0939_FRAME_HEADER_VALUE 0x55U

/**
 * @brief Pass as @p timeout_ms to use the value from bl0939_config_t::default_timeout_ms.
 *
 * Example:
 * @code
 * bl0939_read_measurements(&m, BL0939_TIMEOUT_USE_DEFAULT);
 * @endcode
 */
#define BL0939_TIMEOUT_USE_DEFAULT UINT32_MAX

    /* -------------------------------------------------------------------------
     * Enumerations
     * ---------------------------------------------------------------------- */

    /**
     * @brief Selects which hardware current channel maps to measurements.current_a.
     *
     * The BL0939 provides two independent current inputs (IA, IB). Typical
     * single-phase boards connect only one; split-phase or two-load boards
     * may use both. BL0939_CURRENT_CHANNEL_SUM is useful when you want the
     * combined load current without post-processing in application code.
     */
    typedef enum
    {
        BL0939_CURRENT_CHANNEL_IA = 0,  /**< measurements.current_a reflects IA only. */
        BL0939_CURRENT_CHANNEL_IB = 1,  /**< measurements.current_a reflects IB only. */
        BL0939_CURRENT_CHANNEL_SUM = 2, /**< measurements.current_a = IA + IB (combined load). */
    } bl0939_current_channel_t;

    /* -------------------------------------------------------------------------
     * Configuration & calibration types
     * ---------------------------------------------------------------------- */

    /**
     * @brief Per-board calibration constants for raw-to-engineering conversion.
     *
     * The driver applies the following formulas:
     * @code
     * voltage_v     = (float)v_rms_raw  / calibration.voltage_ref
     * current_X_a   = (float)iX_rms_raw / calibration.current_ref
     * energy_X_kwh  = (float)cfX_cnt    / calibration.energy_ref
     * @endcode
     *
     * Derive these constants from the BL0939 datasheet equations and your
     * shunt/divider component values. All fields must be strictly positive
     * (> 0); the driver rejects configurations that violate this constraint.
     *
     * @note The CF pulse counters (cfa_cnt / cfb_cnt) are hardware pulse counts
     *       accumulated since the last read. They are NOT an on-chip energy
     *       register. energy_ref must account for your pulse-to-kWh scaling.
     */
    typedef struct
    {
        float voltage_ref; /**< Divisor converting v_rms raw counts to volts. */
        float current_ref; /**< Divisor converting i_rms raw counts to amperes. */
        float energy_ref;  /**< Divisor converting CF pulse counts to kWh. */
    } bl0939_calibration_t;

    /**
     * @brief Driver configuration supplied once at bl0939_init().
     *
     * The caller retains ownership of @p uart and must not deinitialize it
     * before calling bl0939_deinit().
     */
    typedef struct
    {
        uart_service_handle_t uart;               /**< Initialized UART handle (required, must not be NULL). */
        bl0939_calibration_t calibration;         /**< Initial calibration factors (all fields > 0). */
        bl0939_current_channel_t current_channel; /**< Channel mapped to measurements.current_a. */
        uint32_t default_timeout_ms;              /**< Timeout used when BL0939_TIMEOUT_USE_DEFAULT is passed. */
        bool auto_request_before_read;            /**< When true, read APIs send a request command before receiving. */
    } bl0939_config_t;

    /* -------------------------------------------------------------------------
     * Data types
     * ---------------------------------------------------------------------- */

    /**
     * @brief Raw 24-bit values decoded verbatim from one BL0939 frame.
     *
     * Use these for calibration, diagnostics, or when you need to apply
     * your own conversion math. For normal application use, prefer
     * bl0939_measurements_t from bl0939_read_measurements().
     *
     * BL0939 encoding notes:
     * - ia_rms / ib_rms / v_rms are unsigned magnitudes (always >= 0).
     * - cfa_cnt / cfb_cnt are signed because the BL0939 outputs a two's
     *   complement value when power flows in the reverse direction (e.g.
     *   during energy export on a grid-tied inverter).
     */
    typedef struct
    {
        uint32_t ia_rms; /**< Raw RMS count for current channel A. */
        uint32_t ib_rms; /**< Raw RMS count for current channel B. */
        uint32_t v_rms;  /**< Raw RMS count for voltage. */
        int32_t cfa_cnt; /**< CF pulse count, channel A (signed; negative = reverse flow). */
        int32_t cfb_cnt; /**< CF pulse count, channel B (signed; negative = reverse flow). */
    } bl0939_raw_data_t;

    /**
     * @brief Engineering-unit measurements for application consumption.
     *
     * Produced by bl0939_read_measurements() after applying bl0939_calibration_t.
     *
     * @note energy_kwh, energy_a_kwh, and energy_b_kwh represent energy derived
     *       from CF pulse counts since the last read, not cumulative totals.
     *       Accumulate them in application code if a running total is required.
     */
    typedef struct
    {
        float voltage_v;    /**< RMS line voltage in volts. */
        float current_a;    /**< Active channel current in amperes (see bl0939_current_channel_t). */
        float current_ia_a; /**< Channel A current in amperes (always populated regardless of active channel). */
        float current_ib_a; /**< Channel B current in amperes (always populated regardless of active channel). */
        float energy_kwh;   /**< Combined energy (A + B) in kWh derived from this frame's CF counts. */
        float energy_a_kwh; /**< Channel A energy in kWh derived from this frame's CF count. */
        float energy_b_kwh; /**< Channel B energy in kWh derived from this frame's CF count. */
    } bl0939_measurements_t;

    /* -------------------------------------------------------------------------
     * Lifecycle API
     * ---------------------------------------------------------------------- */

    /**
     * @brief Initialize the singleton BL0939 driver.
     *
     * Must be called before any other bl0939_* function. Calling this a second
     * time without an intervening bl0939_deinit() returns ESP_ERR_INVALID_STATE.
     *
     * @param[in] config  Non-NULL pointer to a fully populated bl0939_config_t.
     *
     * @retval ESP_OK               Driver initialized successfully.
     * @retval ESP_ERR_INVALID_ARG  @p config is NULL, uart handle is NULL,
     *                              or any calibration factor is <= 0.
     * @retval ESP_ERR_INVALID_STATE Driver was already initialized.
     */
    esp_err_t bl0939_init(const bl0939_config_t *config);

    /**
     * @brief Deinitialize the driver and release internal state.
     *
     * The uart_service handle is *not* deinitialized; UART lifetime is the
     * caller's responsibility.
     *
     * @retval ESP_OK               Driver deinitialized successfully.
     * @retval ESP_ERR_INVALID_STATE Driver was not initialized.
     */
    esp_err_t bl0939_deinit(void);

    /**
     * @brief Query whether the driver has been initialized.
     *
     * Prefer checking the return value of bl0939_* functions over polling this
     * in application code; use this only for assertions or diagnostics.
     *
     * @return true if initialized, false otherwise.
     */
    bool bl0939_is_initialized(void);

    /* -------------------------------------------------------------------------
     * Communication API
     * ---------------------------------------------------------------------- */

    /**
     * @brief Send the BL0939 read-request command over UART.
     *
     * Required before each frame reception unless bl0939_config_t::auto_request_before_read
     * is true, in which case bl0939_read_raw() and bl0939_read_measurements() call
     * this automatically.
     *
     * Call this explicitly only when you need precise control over request timing
     * (e.g. triggering on an external event before reading).
     *
     * @retval ESP_OK               Command sent successfully.
     * @retval ESP_ERR_INVALID_STATE Driver is not initialized.
     * @retval (other)              UART transmission error from uart_service_send().
     */
    esp_err_t bl0939_request_packet(void);

    /* -------------------------------------------------------------------------
     * Read API
     * ---------------------------------------------------------------------- */

    /**
     * @brief Receive and decode one BL0939 frame into raw register values.
     *
     * If bl0939_config_t::auto_request_before_read is true, a request command
     * is sent before waiting for the frame.
     *
     * Frame validation (header byte == 0x55, checksum match) is always performed.
     * A frame that fails either check is discarded and ESP_ERR_INVALID_RESPONSE
     * is returned; no partial data is written to @p out_raw.
     *
     * @param[out] out_raw    Destination for decoded raw values. Must not be NULL.
     * @param[in]  timeout_ms Milliseconds to wait for a complete frame, or
     *                        BL0939_TIMEOUT_USE_DEFAULT.
     *
     * @retval ESP_OK                  Frame received and decoded successfully.
     * @retval ESP_ERR_INVALID_ARG     @p out_raw is NULL.
     * @retval ESP_ERR_INVALID_STATE   Driver is not initialized.
     * @retval ESP_ERR_TIMEOUT         No complete frame arrived within @p timeout_ms.
     * @retval ESP_ERR_INVALID_RESPONSE Frame header or checksum mismatch.
     */
    esp_err_t bl0939_read_raw(bl0939_raw_data_t *out_raw, uint32_t timeout_ms);

    /**
     * @brief Receive one BL0939 frame and return calibrated engineering measurements.
     *
     * Internally calls bl0939_read_raw() then applies the current calibration
     * factors. On any error, @p out_measurements is left unmodified.
     *
     * @param[out] out_measurements  Destination for converted values. Must not be NULL.
     * @param[in]  timeout_ms        Milliseconds to wait, or BL0939_TIMEOUT_USE_DEFAULT.
     *
     * @retval ESP_OK                  Measurements populated successfully.
     * @retval ESP_ERR_INVALID_ARG     @p out_measurements is NULL.
     * @retval ESP_ERR_INVALID_STATE   Driver is not initialized.
     * @retval ESP_ERR_TIMEOUT         No complete frame arrived within @p timeout_ms.
     * @retval ESP_ERR_INVALID_RESPONSE Frame header or checksum mismatch.
     */
    esp_err_t bl0939_read_measurements(bl0939_measurements_t *out_measurements, uint32_t timeout_ms);

    /* -------------------------------------------------------------------------
     * Runtime configuration API
     * ---------------------------------------------------------------------- */

    /**
     * @brief Replace the active calibration factors.
     *
     * Takes effect immediately on the next bl0939_read_measurements() call.
     * Useful when loading calibration from NVS after bl0939_init().
     *
     * @param[in] calibration  Non-NULL pointer; all three fields must be > 0.
     *
     * @retval ESP_OK               Calibration updated.
     * @retval ESP_ERR_INVALID_ARG  @p calibration is NULL or any factor is <= 0.
     * @retval ESP_ERR_INVALID_STATE Driver is not initialized.
     */
    esp_err_t bl0939_set_calibration(const bl0939_calibration_t *calibration);

    /**
     * @brief Read back the currently active calibration factors.
     *
     * Useful for logging, NVS persistence, or verifying a bl0939_set_calibration() call.
     *
     * @param[out] out_calibration  Destination for current calibration. Must not be NULL.
     *
     * @retval ESP_OK               @p out_calibration populated.
     * @retval ESP_ERR_INVALID_ARG  @p out_calibration is NULL.
     * @retval ESP_ERR_INVALID_STATE Driver is not initialized.
     */
    esp_err_t bl0939_get_calibration(bl0939_calibration_t *out_calibration);

    /**
     * @brief Change the current channel mapped to measurements.current_a.
     *
     * Does not affect current_ia_a or current_ib_a, which are always populated.
     *
     * @param[in] channel  BL0939_CURRENT_CHANNEL_IA, _IB, or _SUM.
     *
     * @retval ESP_OK               Channel updated.
     * @retval ESP_ERR_INVALID_ARG  @p channel is not a valid bl0939_current_channel_t value.
     * @retval ESP_ERR_INVALID_STATE Driver is not initialized.
     */
    esp_err_t bl0939_set_current_channel(bl0939_current_channel_t channel);

#ifdef __cplusplus
}
#endif

#endif /* BL0939_H */