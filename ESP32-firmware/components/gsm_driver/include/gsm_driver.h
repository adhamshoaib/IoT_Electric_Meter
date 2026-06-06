/**
 * @file    GSM_driver.h
 * @brief   SIM800 GSM Module Driver
 * @details Provides AT command interface over UART for SIM800 module.
 *          Uses uart_service for ESP32 communication on GPIO32 (TX) / GPIO33 (RX).
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "uart_service.h" /* your existing UART abstraction */

/* ─────────────────────────── Pin & UART Config ─────────────────────────── */

#define GSM_UART_PORT UART_NUM_1 /**< ESP32 UART peripheral         */
#define GSM_TX_PIN 32            /**< ESP32 GPIO → SIM800 RX        */
#define GSM_RX_PIN 33            /**< ESP32 GPIO ← SIM800 TX        */
#define GSM_BAUD_RATE 9600       /**< Default SIM800 baud rate      */

/* ───────────────────────────── Timeouts (ms) ───────────────────────────── */

#define GSM_TIMEOUT_SHORT 3000  /**< Quick commands (AT, echo off) */
#define GSM_TIMEOUT_MEDIUM 5000 /**< Registration, CREG queries    */
#define GSM_TIMEOUT_LONG 15000  /**< SMS send, call setup          */

/* ────────────────────────── Buffer Sizes ───────────────────────────────── */

#define GSM_AT_BUF_SIZE 512    /**< AT command / response buffer  */
#define GSM_SMS_TEXT_MAX 160   /**< Maximum SMS body length       */
#define GSM_PHONE_NUM_MAX 20   /**< E.164 phone-number length     */
#define GSM_UART_RX_BUF_SIZE 1024 /**< UART RX ring buffer size   */
#define GSM_UART_TX_BUF_SIZE 256  /**< UART TX ring buffer size   */

/* ──────────────────────────── Return Codes ─────────────────────────────── */

typedef enum
{
    GSM_OK = 0,           /**< Operation succeeded                      */
    GSM_ERR_TIMEOUT = -1, /**< No response within timeout               */
    GSM_ERR_NO_RESP = -2, /**< Empty / unexpected response              */
    GSM_ERR_SIM = -3,     /**< SIM card error (absent, PIN locked)      */
    GSM_ERR_NOT_REG = -4, /**< Not registered on network                */
    GSM_ERR_GPRS = -5,    /**< GPRS attach or context failure           */
    GSM_ERR_HTTP = -6,    /**< HTTP action failed                       */
    GSM_ERR_CALL = -7,    /**< Call setup failed                        */
    GSM_ERR_SMS = -8,     /**< SMS send failed                          */
    GSM_ERR_PARAM = -9,   /**< Bad parameter passed by caller           */
    GSM_ERR_UART = -10,   /**< UART service error                       */
} gsm_err_t;

/* ─────────────────────────── Network Status ────────────────────────────── */

typedef enum
{
    GSM_NET_NOT_REGISTERED = 0,
    GSM_NET_REGISTERED_HOME = 1,
    GSM_NET_SEARCHING = 2,
    GSM_NET_DENIED = 3,
    GSM_NET_UNKNOWN = 4,
    GSM_NET_REGISTERED_ROAMING = 5,
} gsm_net_status_t;

/* ─────────────────────────── Module Info ───────────────────────────────── */

typedef struct
{
    char manufacturer[32];
    char model[32];
    char revision[32];
    char imei[20];
    char imsi[20];
    int signal_rssi; /**< Raw RSSI value (0–31, 99 = unknown)         */
    int signal_dbm;  /**< Computed dBm value                          */
} gsm_module_info_t;

/* ─────────────────────────── GPRS Config ───────────────────────────────── */

typedef struct
{
    const char *apn;  /**< Access point name, e.g. "internet"          */
    const char *user; /**< APN username (NULL if none)                 */
    const char *pass; /**< APN password  (NULL if none)                */
} gsm_gprs_config_t;

/* ═══════════════════════════════════════════════════════════════════════════
 *  PUBLIC API
 * ═══════════════════════════════════════════════════════════════════════════ */

/* ── Lifecycle ─────────────────────────────────────────────────────────── */

/**
 * @brief  Initialise UART and send basic AT handshake to the SIM800.
 * @return GSM_OK on success, negative gsm_err_t on failure.
 */
gsm_err_t gsm_init(void);

/**
 * @brief  Release UART and reset internal state.
 */
void gsm_deinit(void);

/**
 * @brief  Wait for the "SMS Ready" unsolicited message after power-on.
 *         Polls UART at 500ms intervals; the URC may be missed if it arrives
 *         between reads. gsm_init() already calls this internally, so this
 *         function is only needed if re-initialising manually.
 * @param[in]  timeout_ms  Maximum wait in milliseconds.
 * @return GSM_OK when ready, GSM_ERR_TIMEOUT otherwise.
 */
gsm_err_t gsm_wait_for_ready(uint32_t timeout_ms);

/**
 * @brief  Send a raw AT command and capture the response.
 *         Returns GSM_OK even if the module replies "ERROR" — the caller
 *         must verify the response content (use gsm_cmd_ok() internally
 *         or check for "OK" / expected pattern in resp_buf).
 * @param[in]  cmd        NULL-terminated AT string (e.g. "AT+CREG?").
 * @param[out] resp_buf   Caller-allocated buffer for the response.
 * @param[in]  buf_size   Size of resp_buf.
 * @param[in]  timeout_ms Wait time in milliseconds.
 * @return GSM_OK on UART success, GSM_ERR_TIMEOUT, or GSM_ERR_NO_RESP.
 */
gsm_err_t gsm_send_at(const char *cmd,
                      char *resp_buf,
                      size_t buf_size,
                      uint32_t timeout_ms);

/**
 * @brief  Send a command and check that the response contains "OK".
 * @param[in]  cmd        NULL-terminated AT string.
 * @param[in]  timeout_ms Wait time in milliseconds.
 * @return GSM_OK or negative error code.
 */
gsm_err_t gsm_cmd_ok(const char *cmd, uint32_t timeout_ms);

/* ── Module Info ───────────────────────────────────────────────────────── */

/**
 * @brief  Populate a gsm_module_info_t with manufacturer, model, IMEI, etc.
 * @param[out] info  Pointer to caller-allocated structure.
 * @return GSM_OK or negative error code.
 */
gsm_err_t gsm_get_module_info(gsm_module_info_t *info);

/**
 * @brief  Read RSSI and map it to dBm.
 * @param[out] rssi  Raw RSSI (0–31).
 * @param[out] dbm   Signal level in dBm.
 * @return GSM_OK or GSM_ERR_TIMEOUT.
 */
gsm_err_t gsm_get_signal_quality(int *rssi, int *dbm);

/* ── Network Registration ──────────────────────────────────────────────── */

/**
 * @brief  Query current network registration status.
 * @param[out] status  One of the gsm_net_status_t values.
 * @return GSM_OK or negative error code.
 */
gsm_err_t gsm_get_network_status(gsm_net_status_t *status);

/**
 * @brief  Block until registered on home or roaming network.
 * @param[in]  timeout_ms  Maximum wait in milliseconds.
 * @return GSM_OK when registered, GSM_ERR_TIMEOUT otherwise.
 */
gsm_err_t gsm_wait_for_registration(uint32_t timeout_ms);

/* ── SMS ───────────────────────────────────────────────────────────────── */

/**
 * @brief  Send an SMS message in text mode.
 * @param[in]  phone_number  Destination in E.164 format, e.g. "+201001234567".
 * @param[in]  message       NULL-terminated message body (max 160 chars).
 * @return GSM_OK, GSM_ERR_SMS, or GSM_ERR_TIMEOUT.
 */
gsm_err_t gsm_sms_send(const char *phone_number, const char *message);

/* ── Voice Call ────────────────────────────────────────────────────────── */

/**
 * @brief  Initiate a voice call.
 * @param[in]  phone_number  Destination in E.164 format.
 * @return GSM_OK when call is dialling, or negative error code.
 */
gsm_err_t gsm_call_dial(const char *phone_number);

/**
 * @brief  Hang up an active or dialling call.
 * @return GSM_OK or negative error code.
 */
gsm_err_t gsm_call_hangup(void);

/* ── Utility ───────────────────────────────────────────────────────────── */

/**
 * @brief  Convert a raw RSSI value to dBm.
 * @param  rssi  Value returned by AT+CSQ (0–31).
 * @return Signal level in dBm, or 0 if rssi == 99 (unknown).
 */
int gsm_rssi_to_dbm(int rssi);

/**
 * @brief  Return a human-readable string for a gsm_err_t code.
 */
const char *gsm_err_to_str(gsm_err_t err);