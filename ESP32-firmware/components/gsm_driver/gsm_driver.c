/**
 * @file    GSM_driver.c
 * @brief   SIM800 GSM Module Driver – Implementation
 *
 * Communicates with the SIM800 via uart_service on:
 *   TX → GPIO32   RX ← GPIO33   (UART_NUM_1, 9600 8N1)
 *
 * All public functions are declared in GSM_driver.h.
 */

#include "gsm_driver.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/* ────────────────────────────────────────────────────────────────────────── *
 *  Private helpers & macros
 * ────────────────────────────────────────────────────────────────────────── */

static const char *TAG = "GSM_DRV";

#define GSM_CRLF "\r\n"
#define GSM_CTRL_Z "\x1A"

/** Milliseconds → FreeRTOS ticks */
#define MS2TICK(ms) ((ms) / portTICK_PERIOD_MS)

/** Swallow VS-Code unused-variable warnings in release builds */
#define GSM_UNUSED(x) ((void)(x))

/* Internal state ----------------------------------------------------------- */
static bool s_initialised = false;
static uart_service_handle_t s_uart = NULL;

/* ────────────────────────────────────────────────────────────────────────── *
 *  Low-level UART helpers (wraps uart_service)
 * ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Flush the RX ring-buffer so stale bytes don't poison the next read.
 */
static void gsm_uart_flush(void)
{
    uart_service_flush_input(s_uart);
}

/**
 * @brief Wait briefly, then drain all pending RX data (matches working pattern).
 *        Call before sending critical AT commands to avoid stale data confusion.
 */
static void gsm_drain(uint32_t wait_ms)
{
    uint8_t tmp[64];
    vTaskDelay(pdMS_TO_TICKS(wait_ms));
    size_t n = 0;
    do
    {
        n = 0;
        uart_service_read(s_uart, tmp, sizeof(tmp) - 1, &n, pdMS_TO_TICKS(200));
    } while (n > 0);
    gsm_uart_flush();
}

/**
 * @brief Write a NULL-terminated string followed by CR+LF.
 */
static gsm_err_t gsm_uart_write_line(const char *line)
{
    if (uart_service_send(s_uart,
                          (const uint8_t *)line,
                          strlen(line)) != ESP_OK)
    {
        return GSM_ERR_UART;
    }
    if (uart_service_send(s_uart,
                          (const uint8_t *)GSM_CRLF,
                          2) != ESP_OK)
    {
        return GSM_ERR_UART;
    }
    return GSM_OK;
}

/**
 * @brief Read response from UART with a single blocking call.
 *        Matches the proven approach from the SIM800 diagnostic test.
 * @return Number of bytes placed in buf, or -1 on UART error.
 */
static int gsm_uart_read(char *buf, size_t buf_size, uint32_t timeout_ms)
{
    if (!buf || buf_size == 0)
        return -1;

    size_t n = 0;
    esp_err_t e = uart_service_read(s_uart, (uint8_t *)buf,
                                    buf_size - 1, &n,
                                    timeout_ms);
    if (e != ESP_OK)
        return -1;

    buf[n] = '\0';
    return (int)n;
}

/* ────────────────────────────────────────────────────────────────────────── *
 *  AT command engine
 * ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Send an AT command and read back the complete response.
 *
 * The function waits until it sees "OK", "ERROR", or the caller's
 * timeout expires.
 */
gsm_err_t gsm_send_at(const char *cmd,
                      char *resp_buf,
                      size_t buf_size,
                      uint32_t timeout_ms)
{
    if (!cmd || !resp_buf || buf_size < 2)
        return GSM_ERR_PARAM;

    /* Drain pending URCs (e.g. "Call Ready", "+CPIN: READY") before flush */
    {
        uint8_t urc_drain[64];
        size_t n = 0;
        uart_service_read(s_uart, urc_drain, sizeof(urc_drain), &n, 50);
    }

    gsm_uart_flush();

    gsm_err_t wr = gsm_uart_write_line(cmd);
    if (wr != GSM_OK)
        return wr;

    int n = gsm_uart_read(resp_buf, buf_size, timeout_ms);
    if (n < 0)
        return GSM_ERR_UART;
    if (n == 0)
        return GSM_ERR_TIMEOUT;

    ESP_LOGD(TAG, "AT> %s", cmd);
    ESP_LOGD(TAG, "AT< %s", resp_buf);

    return GSM_OK;
}

/**
 * @brief Send a command and check whether the response contains "OK".
 */
static gsm_err_t gsm_cmd_ok(const char *cmd, uint32_t timeout_ms)
{
    char buf[GSM_AT_BUF_SIZE];
    gsm_err_t err = gsm_send_at(cmd, buf, sizeof(buf), timeout_ms);
    if (err != GSM_OK)
        return err;
    return (strstr(buf, "OK") != NULL) ? GSM_OK : GSM_ERR_NO_RESP;
}

/* ────────────────────────────────────────────────────────────────────────── *
 *  Utility – extract first line before \n
 * ────────────────────────────────────────────────────────────────────────── */

static void extract_first_line(char *buf, char *dest, size_t dest_size)
{
    dest[0] = '\0';
    char *nl = strchr(buf, '\n');
    if (nl)
    {
        size_t len = (size_t)(nl - buf);
        if (len >= dest_size)
            len = dest_size - 1;
        strncpy(dest, buf, len);
        dest[len] = '\0';
    }
}

/* ────────────────────────────────────────────────────────────────────────── *
 *  Lifecycle
 * ────────────────────────────────────────────────────────────────────────── */

gsm_err_t gsm_init(void)
{
    if (s_initialised)
        return GSM_OK;

    /* Configure UART via uart_service ------------------------------------ */
    uart_service_config_t cfg = {
        .port = GSM_UART_PORT,
        .baud_rate = GSM_BAUD_RATE,
        .tx_pin = GSM_TX_PIN,
        .rx_pin = GSM_RX_PIN,
        .rx_buffer_size = GSM_UART_RX_BUF_SIZE,
        .tx_buffer_size = GSM_UART_TX_BUF_SIZE,
    };

    if (uart_service_init(&cfg, &s_uart) != ESP_OK)
    {
        return GSM_ERR_UART;
    }

    /* Allow SIM800 to stabilise after power-on */
    vTaskDelay(MS2TICK(5000));

    /* Drain boot-up messages (e.g. "RDY", "SMS Ready") */
    gsm_drain(500);

    /* Wait for "SMS Ready" unsolicited URC (up to 30 s) */
    {
        uint32_t elapsed = 0;
        bool ready = false;
        while (elapsed < 30000)
        {
            char buf[128];
            size_t n = 0;
            esp_err_t e = uart_service_read(s_uart, (uint8_t *)buf,
                                            sizeof(buf) - 1, &n, 500);
            if (e == ESP_OK && n > 0)
            {
                buf[n] = '\0';
                ESP_LOGD(TAG, "boot: %s", buf);
                if (strstr(buf, "SMS Ready"))
                {
                    ready = true;
                    break;
                }
            }
            elapsed += 500;
        }
        if (!ready)
            ESP_LOGW(TAG, "SMS Ready not detected (module may still work)");
        gsm_drain(500);
    }

    /* Basic handshake: retry with drain between attempts */
    gsm_err_t err;
    for (int attempt = 0; attempt < 5; attempt++)
    {
        err = gsm_cmd_ok("AT", GSM_TIMEOUT_SHORT);
        if (err == GSM_OK)
            break;
        gsm_drain(300);
        if (attempt == 4)
        {
            uart_service_deinit(&s_uart);
            return GSM_ERR_TIMEOUT;
        }
    }

    /* Disable echo – retry with verification using a test AT command */
    for (int i = 0; i < 5; i++)
    {
        {
            char buf[64];
            gsm_send_at("ATE0", buf, sizeof(buf), GSM_TIMEOUT_SHORT);
        }
        gsm_drain(500);
        {
            char test_buf[64];
            gsm_send_at("AT", test_buf, sizeof(test_buf), GSM_TIMEOUT_SHORT);
            /* If echo is OFF, response is "OK" preceded by \r\n (no command echo) */
            if (strstr(test_buf, "OK") && !strstr(test_buf, "AT\r\n"))
                break;
        }
    }
    gsm_drain(500);

    /* Enable verbose error codes */
    gsm_cmd_ok("AT+CMEE=2", GSM_TIMEOUT_SHORT);
    gsm_drain(300);

    /* Check SIM is ready */
    {
        char buf[64];
        gsm_err_t e = gsm_send_at("AT+CPIN?", buf, sizeof(buf), GSM_TIMEOUT_SHORT);
        if (e == GSM_OK && strstr(buf, "+CPIN: READY") == NULL)
        {
            uart_service_deinit(&s_uart);
            return GSM_ERR_SIM;
        }
    }

    s_initialised = true;
    ESP_LOGI(TAG, "SIM800 initialised");
    return GSM_OK;
}

void gsm_deinit(void)
{
    if (!s_initialised)
        return;
    uart_service_deinit(&s_uart);
    s_initialised = false;
}

gsm_err_t gsm_wait_for_ready(uint32_t timeout_ms)
{
    uint32_t elapsed = 0;
    const uint32_t POLL = 500;
    gsm_drain(100);
    while (elapsed < timeout_ms)
    {
        char buf[128];
        size_t n = 0;
        esp_err_t e = uart_service_read(s_uart, (uint8_t *)buf,
                                        sizeof(buf) - 1, &n, POLL);
        if (e == ESP_OK && n > 0)
        {
            buf[n] = '\0';
            if (strstr(buf, "SMS Ready"))
            {
                gsm_drain(500);
                return GSM_OK;
            }
        }
        elapsed += POLL;
    }
    gsm_drain(500);
    return GSM_ERR_TIMEOUT;
}

/* ────────────────────────────────────────────────────────────────────────── *
 *  Module Info
 * ────────────────────────────────────────────────────────────────────────── */

gsm_err_t gsm_get_module_info(gsm_module_info_t *info)
{
    if (!info)
        return GSM_ERR_PARAM;

    memset(info, 0, sizeof(*info));

    char buf[GSM_AT_BUF_SIZE];

    /* Manufacturer — try 3GPP (AT+CGMI) first, fall back to GSM 07.07 (AT+GMI) */
    if (gsm_send_at("AT+CGMI", buf, sizeof(buf), GSM_TIMEOUT_SHORT) == GSM_OK)
        extract_first_line(buf, info->manufacturer, sizeof(info->manufacturer));
    if (info->manufacturer[0] == '\0')
    {
        if (gsm_send_at("AT+GMI", buf, sizeof(buf), GSM_TIMEOUT_SHORT) == GSM_OK)
            extract_first_line(buf, info->manufacturer, sizeof(info->manufacturer));
    }

    /* Model — AT+CGMM fallback AT+GMM */
    if (gsm_send_at("AT+CGMM", buf, sizeof(buf), GSM_TIMEOUT_SHORT) == GSM_OK)
        extract_first_line(buf, info->model, sizeof(info->model));
    if (info->model[0] == '\0')
    {
        if (gsm_send_at("AT+GMM", buf, sizeof(buf), GSM_TIMEOUT_SHORT) == GSM_OK)
            extract_first_line(buf, info->model, sizeof(info->model));
    }

    /* Firmware revision — AT+CGMR fallback AT+GMR */
    if (gsm_send_at("AT+CGMR", buf, sizeof(buf), GSM_TIMEOUT_SHORT) == GSM_OK)
        extract_first_line(buf, info->revision, sizeof(info->revision));
    if (info->revision[0] == '\0')
    {
        if (gsm_send_at("AT+GMR", buf, sizeof(buf), GSM_TIMEOUT_SHORT) == GSM_OK)
            extract_first_line(buf, info->revision, sizeof(info->revision));
    }

    /* IMEI — AT+CGSN fallback AT+GSN */
    if (gsm_send_at("AT+CGSN", buf, sizeof(buf), GSM_TIMEOUT_SHORT) == GSM_OK)
        extract_first_line(buf, info->imei, sizeof(info->imei));
    if (info->imei[0] == '\0')
    {
        if (gsm_send_at("AT+GSN", buf, sizeof(buf), GSM_TIMEOUT_SHORT) == GSM_OK)
            extract_first_line(buf, info->imei, sizeof(info->imei));
    }

    /* IMSI */
    if (gsm_send_at("AT+CIMI", buf, sizeof(buf), GSM_TIMEOUT_SHORT) == GSM_OK)
        extract_first_line(buf, info->imsi, sizeof(info->imsi));

    /* Signal quality */
    gsm_get_signal_quality(&info->signal_rssi, &info->signal_dbm);

    return GSM_OK;
}

gsm_err_t gsm_get_signal_quality(int *rssi, int *dbm)
{
    if (!rssi || !dbm)
        return GSM_ERR_PARAM;

    char buf[GSM_AT_BUF_SIZE];
    gsm_err_t err = gsm_send_at("AT+CSQ", buf, sizeof(buf), GSM_TIMEOUT_SHORT);
    if (err != GSM_OK)
        return err;

    /* Response: +CSQ: <rssi>,<ber> */
    char *p = strstr(buf, "+CSQ:");
    if (!p)
        return GSM_ERR_NO_RESP;

    int raw_rssi = 99, raw_ber = 0;
    sscanf(p, "+CSQ: %d,%d", &raw_rssi, &raw_ber);
    GSM_UNUSED(raw_ber);

    *rssi = raw_rssi;
    *dbm = gsm_rssi_to_dbm(raw_rssi);
    return GSM_OK;
}

/* ────────────────────────────────────────────────────────────────────────── *
 *  Network registration
 * ────────────────────────────────────────────────────────────────────────── */

gsm_err_t gsm_get_network_status(gsm_net_status_t *status)
{
    if (!status)
        return GSM_ERR_PARAM;

    char buf[GSM_AT_BUF_SIZE];
    gsm_err_t err = gsm_send_at("AT+CREG?", buf, sizeof(buf), GSM_TIMEOUT_SHORT);
    if (err != GSM_OK)
        return err;

    /* Response: +CREG: <n>,<stat>  or  +CREG: <stat> */
    char *p = strstr(buf, "+CREG:");
    if (!p)
        return GSM_ERR_NO_RESP;

    int n = 0, stat = 0;
    int parsed = sscanf(p, "+CREG: %d,%d", &n, &stat);
    if (parsed < 2)
        sscanf(p, "+CREG: %d", &stat);

    static const char *reg_desc[] = {
        "not registered",
        "registered (home)",
        "searching",
        "denied",
        "unknown",
        "registered (roaming)"};
    const char *desc = (stat >= 0 && stat <= 5) ? reg_desc[stat] : "?";
    ESP_LOGI(TAG, "CREG status %d: %s", stat, desc);

    *status = (gsm_net_status_t)stat;
    return GSM_OK;
}

gsm_err_t gsm_wait_for_registration(uint32_t timeout_ms)
{
    uint32_t elapsed = 0;
    const uint32_t POLL = 2000;

    while (elapsed < timeout_ms)
    {
        gsm_net_status_t s;
        if (gsm_get_network_status(&s) == GSM_OK)
        {
            if (s == GSM_NET_REGISTERED_HOME || s == GSM_NET_REGISTERED_ROAMING)
                return GSM_OK;
        }
        vTaskDelay(MS2TICK(POLL));
        elapsed += POLL;
    }
    return GSM_ERR_TIMEOUT;
}

/* ────────────────────────────────────────────────────────────────────────── *
 *  SMS
 * ────────────────────────────────────────────────────────────────────────── */

gsm_err_t gsm_sms_send(const char *phone_number, const char *message)
{
    if (!phone_number || !message)
        return GSM_ERR_PARAM;
    if (strlen(message) > GSM_SMS_TEXT_MAX)
        return GSM_ERR_PARAM;

    char buf[GSM_AT_BUF_SIZE];
    gsm_err_t err;

    /* Switch to text mode */
    err = gsm_cmd_ok("AT+CMGF=1", GSM_TIMEOUT_SHORT);
    if (err != GSM_OK)
        return GSM_ERR_SMS;

    /* Set destination */
    char cmd[GSM_AT_BUF_SIZE];
    snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"", phone_number);

    gsm_uart_flush();
    if (gsm_uart_write_line(cmd) != GSM_OK)
        return GSM_ERR_UART;

    /* Wait for the '>' prompt */
    int n = gsm_uart_read(buf, sizeof(buf), GSM_TIMEOUT_SHORT);
    if (n <= 0 || strstr(buf, ">") == NULL)
        return GSM_ERR_SMS;

    /* Send the message body, terminated by Ctrl-Z */
    uart_service_send(s_uart, (const uint8_t *)message, strlen(message));
    uart_service_send(s_uart, (const uint8_t *)GSM_CTRL_Z, 1);

    /* Wait for "+CMGS:" confirmation */
    n = gsm_uart_read(buf, sizeof(buf), GSM_TIMEOUT_LONG);
    if (n <= 0)
        return GSM_ERR_TIMEOUT;
    if (strstr(buf, "+CMGS:") == NULL)
        return GSM_ERR_SMS;

    return GSM_OK;
}

/* ────────────────────────────────────────────────────────────────────────── *
 *  Voice Call
 * ────────────────────────────────────────────────────────────────────────── */

gsm_err_t gsm_call_dial(const char *phone_number)
{
    if (!phone_number)
        return GSM_ERR_PARAM;

    char cmd[GSM_AT_BUF_SIZE];
    snprintf(cmd, sizeof(cmd), "ATD%s;", phone_number); /* ';' = voice call */

    char buf[GSM_AT_BUF_SIZE];
    gsm_err_t err = gsm_send_at(cmd, buf, sizeof(buf), GSM_TIMEOUT_LONG);
    if (err != GSM_OK)
        return err;

    if (strstr(buf, "OK") || strstr(buf, "CONNECT"))
        return GSM_OK;
    return GSM_ERR_CALL;
}

gsm_err_t gsm_call_hangup(void)
{
    return gsm_cmd_ok("ATH", GSM_TIMEOUT_SHORT);
}

/* ────────────────────────────────────────────────────────────────────────── *
 *  GPRS
 * ────────────────────────────────────────────────────────────────────────── */

static bool s_gprs_connected = false;

gsm_err_t gsm_gprs_connect(const gsm_gprs_config_t *config)
{
    if (!config || !config->apn)
        return GSM_ERR_PARAM;

    char cmd[GSM_AT_BUF_SIZE];
    char buf[GSM_AT_BUF_SIZE];

    /* Close any existing bearer */
    gsm_cmd_ok("AT+SAPBR=0,1", GSM_TIMEOUT_MEDIUM);
    vTaskDelay(MS2TICK(500));

    /* Bearer type = GPRS */
    gsm_cmd_ok("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", GSM_TIMEOUT_SHORT);

    /* APN */
    snprintf(cmd, sizeof(cmd), "AT+SAPBR=3,1,\"APN\",\"%s\"", config->apn);
    gsm_cmd_ok(cmd, GSM_TIMEOUT_SHORT);

    /* Optional credentials */
    if (config->user)
    {
        snprintf(cmd, sizeof(cmd), "AT+SAPBR=3,1,\"USER\",\"%s\"", config->user);
        gsm_cmd_ok(cmd, GSM_TIMEOUT_SHORT);
    }
    if (config->pass)
    {
        snprintf(cmd, sizeof(cmd), "AT+SAPBR=3,1,\"PWD\",\"%s\"", config->pass);
        gsm_cmd_ok(cmd, GSM_TIMEOUT_SHORT);
    }

    /* Open bearer */
    gsm_err_t err = gsm_send_at("AT+SAPBR=1,1",
                                buf, sizeof(buf),
                                GSM_TIMEOUT_GPRS);
    if (err != GSM_OK)
        return GSM_ERR_GPRS;
    if (!strstr(buf, "OK"))
        return GSM_ERR_GPRS;

    /* Confirm we have an IP */
    err = gsm_send_at("AT+SAPBR=2,1", buf, sizeof(buf), GSM_TIMEOUT_SHORT);
    if (err != GSM_OK)
        return GSM_ERR_GPRS;

    /* Response: +SAPBR: 1,1,"<ip>" — status field == 1 means connected */
    char *p = strstr(buf, "+SAPBR:");
    if (!p)
        return GSM_ERR_GPRS;

    int cid, status;
    sscanf(p, "+SAPBR: %d,%d", &cid, &status);
    if (status != 1)
        return GSM_ERR_GPRS;

    s_gprs_connected = true;
    return GSM_OK;
}

gsm_err_t gsm_gprs_disconnect(void)
{
    gsm_err_t err = gsm_cmd_ok("AT+SAPBR=0,1", GSM_TIMEOUT_MEDIUM);
    s_gprs_connected = false;
    return err;
}

bool gsm_gprs_is_connected(void)
{
    if (!s_gprs_connected)
        return false;

    char buf[GSM_AT_BUF_SIZE];
    gsm_err_t err = gsm_send_at("AT+SAPBR=2,1", buf, sizeof(buf), GSM_TIMEOUT_SHORT);
    if (err != GSM_OK)
    {
        s_gprs_connected = false;
        return false;
    }

    char *p = strstr(buf, "+SAPBR:");
    if (!p)
    {
        s_gprs_connected = false;
        return false;
    }

    int cid, status;
    sscanf(p, "+SAPBR: %d,%d", &cid, &status);
    if (status != 1)
        s_gprs_connected = false;
    return (status == 1);
}

/* ────────────────────────────────────────────────────────────────────────── *
 *  HTTP
 * ────────────────────────────────────────────────────────────────────────── */

/** Shared setup used by both GET and POST */
static gsm_err_t gsm_http_init(const char *url)
{
    char cmd[GSM_HTTP_URL_MAX + 32];

    /* Close any previous session, then drain (matches working pattern) */
    gsm_cmd_ok("AT+HTTPTERM", GSM_TIMEOUT_SHORT);
    vTaskDelay(MS2TICK(200));
    gsm_drain(300);

    if (gsm_cmd_ok("AT+HTTPINIT", GSM_TIMEOUT_SHORT) != GSM_OK)
    {
        ESP_LOGE(TAG, "AT+HTTPINIT failed");
        return GSM_ERR_HTTP;
    }
    if (gsm_cmd_ok("AT+HTTPPARA=\"CID\",1", GSM_TIMEOUT_SHORT) != GSM_OK)
    {
        ESP_LOGE(TAG, "AT+HTTPPARA CID failed");
        return GSM_ERR_HTTP;
    }

    /* Enable SSL if supported — SIM800L standard firmware does NOT support
     * SSL, but some modules with updated firmware do.
     *
     * Some modules falsely return OK to AT+HTTPSSL=1 without being able
     * to complete a real TLS handshake, causing an ~28 s timeout later.
     * CONFIG_GSM_FORCE_HTTP bypasses the detection and forces HTTP. */
#if CONFIG_GSM_FORCE_HTTP
    (void)gsm_cmd_ok("AT+HTTPSSL=0", GSM_TIMEOUT_SHORT);
    bool ssl_ok = false;
    ESP_LOGI(TAG, "GSM_FORCE_HTTP enabled — SSL disabled by config");
#else
    bool ssl_ok = (gsm_cmd_ok("AT+HTTPSSL=1", GSM_TIMEOUT_SHORT) == GSM_OK);
    if (!ssl_ok)
        ESP_LOGW(TAG, "SSL not available — will use HTTP");
#endif

    /* Enable redirect following only when SSL is available; otherwise a
     * server redirect from HTTP→HTTPS would fail. */
    gsm_cmd_ok(ssl_ok ? "AT+HTTPPARA=\"REDIR\",\"1\""
                      : "AT+HTTPPARA=\"REDIR\",\"0\"",
               GSM_TIMEOUT_SHORT);

    /* Set URL — transparently downgrade https:// to http:// when SSL
     * is not available so callers can always pass the canonical URL. */
    const char *effective_url = url;
    char http_url[GSM_HTTP_URL_MAX];
    if (!ssl_ok && strncmp(url, "https://", 8) == 0)
    {
        snprintf(http_url, sizeof(http_url), "http://%s", url + 8);
        effective_url = http_url;
        ESP_LOGI(TAG, "Downgraded URL to HTTP (SSL unavailable)");
    }

    snprintf(cmd, sizeof(cmd), "AT+HTTPPARA=\"URL\",\"%s\"", effective_url);
    if (gsm_cmd_ok(cmd, GSM_TIMEOUT_SHORT) != GSM_OK)
    {
        ESP_LOGE(TAG, "AT+HTTPPARA URL failed: %s", effective_url);
        return GSM_ERR_HTTP;
    }

    /* Verify the GPRS bearer is still active — if it dropped between the
     * GPRS setup and now, HTTPDATA would fail with "operation not allowed". */
    {
        char sapbr_buf[64];
        gsm_err_t sapbr_err = gsm_send_at("AT+SAPBR=2,1", sapbr_buf,
                                          sizeof(sapbr_buf), GSM_TIMEOUT_SHORT);
        if (sapbr_err != GSM_OK)
        {
            ESP_LOGW(TAG, "SAPBR query failed — bearer may have dropped");
            return GSM_ERR_GPRS;
        }
        char *p = strstr(sapbr_buf, "+SAPBR:");
        if (p)
        {
            int cid, status;
            if (sscanf(p, "+SAPBR: %d,%d", &cid, &status) == 2 && status != 1)
            {
                ESP_LOGW(TAG, "Bearer not active (status=%d) — need GPRS reconnect", status);
                return GSM_ERR_GPRS;
            }
        }
    }

    return GSM_OK;
}

/** Parse status code and read body after AT+HTTPACTION */
static gsm_err_t gsm_http_read_response(gsm_http_response_t *response)
{
    char buf[GSM_AT_BUF_SIZE];
    uint32_t elapsed = 0;
    const uint32_t POLL_MS = 500;

    response->status_code = 0;
    response->data_len = 0;
    response->body[0] = '\0';

    /* Poll until +HTTPACTION: appears — it arrives as a URC after OK */
    while (elapsed < GSM_TIMEOUT_HTTP)
    {
        int n = gsm_uart_read(buf, sizeof(buf), POLL_MS);
        if (n > 0)
        {
            buf[n] = '\0';
            ESP_LOGD(TAG, "HTTP raw (len=%d): %.*s", n, n > 200 ? 200 : n, buf);

            char *p = strstr(buf, "+HTTPACTION:");
            if (p)
            {
                int method, status, data_len;
                int parsed = sscanf(p, "+HTTPACTION: %d,%d,%d", &method, &status, &data_len);
                GSM_UNUSED(method);

                if (parsed < 3)
                {
                    ESP_LOGW(TAG, "Malformed +HTTPACTION: %.*s", 80, p);
                    return GSM_ERR_HTTP;
                }

                ESP_LOGD(TAG, "HTTP status=%d, len=%d (elapsed=%u ms)",
                         status, data_len, elapsed);

                response->status_code = status;
                response->data_len = data_len;

                if (data_len <= 0)
                    return GSM_OK;

                /* Read body */
                char read_cmd[32];
                int to_read = data_len < (GSM_HTTP_RESP_MAX - 1) ? data_len : (GSM_HTTP_RESP_MAX - 1);
                snprintf(read_cmd, sizeof(read_cmd), "AT+HTTPREAD=0,%d", to_read);

                char resp[GSM_HTTP_RESP_MAX + 64];
                gsm_err_t err = gsm_send_at(read_cmd, resp, sizeof(resp), GSM_TIMEOUT_MEDIUM);
                if (err != GSM_OK)
                    return err;

                /* Locate the "+HTTPREAD:" header to find body start */
                char *header = strstr(resp, "+HTTPREAD:");
                char *body_start = header ? strchr(header, '\n') : NULL;
                if (body_start)
                {
                    body_start++;
                    while (*body_start == '\r')
                        body_start++;
                }
                else
                {
                    body_start = resp;
                }

                /* Copy exactly data_len bytes to avoid trailing \r\nOK\r\n */
                size_t copy_len = (data_len > 0 && (size_t)data_len < GSM_HTTP_RESP_MAX)
                                      ? (size_t)data_len
                                      : GSM_HTTP_RESP_MAX - 1;
                memcpy(response->body, body_start, copy_len);
                response->body[copy_len] = '\0';

                return GSM_OK;
            }
        }
        elapsed += POLL_MS;
    }

    ESP_LOGW(TAG, "+HTTPACTION not found after %u ms, last buf: %.*s",
             GSM_TIMEOUT_HTTP, 120, buf);
    return GSM_ERR_TIMEOUT;
}

gsm_err_t gsm_http_get(const char *url, gsm_http_response_t *response)
{
    if (!url || !response)
        return GSM_ERR_PARAM;

    gsm_err_t err = gsm_http_init(url);
    if (err != GSM_OK)
        return err;

    /* Trigger GET (method = 0) — send directly to avoid gsm_cmd_ok
     * consuming the +HTTPACTION URC */
    gsm_uart_flush();
    if (gsm_uart_write_line("AT+HTTPACTION=0") != GSM_OK)
    {
        gsm_cmd_ok("AT+HTTPTERM", GSM_TIMEOUT_SHORT);
        return GSM_ERR_UART;
    }

    err = gsm_http_read_response(response);
    if (err != GSM_OK)
    {
        gsm_cmd_ok("AT+HTTPTERM", GSM_TIMEOUT_SHORT);
        return err;
    }

    gsm_cmd_ok("AT+HTTPTERM", GSM_TIMEOUT_SHORT);
    return GSM_OK;
}

gsm_err_t gsm_http_post(const char *url,
                        const char *body,
                        size_t body_len,
                        const char *content_type,
                        const char *headers,
                        gsm_http_response_t *response)
{
    if (!url || !body || !response)
        return GSM_ERR_PARAM;

    gsm_err_t err = gsm_http_init(url);
    if (err != GSM_OK)
        return err;

    /* Content-type */
    {
        const char *ct = content_type ? content_type : "application/x-www-form-urlencoded";
        char ct_cmd[128];
        snprintf(ct_cmd, sizeof(ct_cmd), "AT+HTTPPARA=\"CONTENT\",\"%s\"", ct);
        gsm_err_t ct_err = gsm_cmd_ok(ct_cmd, GSM_TIMEOUT_SHORT);
        if (ct_err != GSM_OK)
        {
            ESP_LOGW(TAG, "CONTENT not set (%s) — state may be corrupted",
                     gsm_err_to_str(ct_err));
        }
    }

    /* Extra headers via USERDATA (may fail on some firmware) */
    if (headers)
    {
        char hdr_cmd[256];
        snprintf(hdr_cmd, sizeof(hdr_cmd), "AT+HTTPPARA=\"USERDATA\",\"%s\"", headers);
        gsm_err_t hdr_err = gsm_cmd_ok(hdr_cmd, GSM_TIMEOUT_SHORT);
        if (hdr_err != GSM_OK)
        {
            ESP_LOGW(TAG, "USERDATA not accepted (%s) — continuing without custom headers",
                     gsm_err_to_str(hdr_err));
        }
    }

    /* Verify bearer is still alive — the HTTP init + PARA setup above can
     * take 15+ seconds, during which the GPRS bearer may drop (SIM800
     * inactivity timeout ~30 s).  Check before committing to HTTPDATA. */
    {
        char sapbr_buf[64];
        gsm_err_t sapbr_err = gsm_send_at("AT+SAPBR=2,1", sapbr_buf,
                                          sizeof(sapbr_buf), GSM_TIMEOUT_SHORT);
        if (sapbr_err == GSM_OK)
        {
            char *p = strstr(sapbr_buf, "+SAPBR:");
            if (p)
            {
                int cid, status;
                if (sscanf(p, "+SAPBR: %d,%d", &cid, &status) == 2 && status != 1)
                {
                    ESP_LOGW(TAG, "Bearer dropped before HTTPDATA (status=%d) — need GPRS reconnect", status);
                    s_gprs_connected = false;
                    gsm_cmd_ok("AT+HTTPTERM", GSM_TIMEOUT_SHORT);
                    return GSM_ERR_GPRS;
                }
            }
        }
    }

    /* Send body length, wait for '>' prompt, then write data */
    char cmd[32];
    char buf[GSM_AT_BUF_SIZE];
    snprintf(cmd, sizeof(cmd), "AT+HTTPDATA=%u,%u",
             (unsigned)body_len,
             (unsigned)(GSM_TIMEOUT_MEDIUM / 1000));

    int n;
    bool httpdata_ok = false;
    for (int retry = 0; retry < 2; retry++)
    {
        gsm_uart_flush();
        if (gsm_uart_write_line(cmd) != GSM_OK)
            return GSM_ERR_UART;

        n = gsm_uart_read(buf, sizeof(buf), GSM_TIMEOUT_SHORT);
        if (n > 0 && strstr(buf, "DOWNLOAD") != NULL)
        {
            uart_service_send(s_uart, (const uint8_t *)body, body_len);
            vTaskDelay(MS2TICK(200));
            n = gsm_uart_read(buf, sizeof(buf), GSM_TIMEOUT_MEDIUM);
            if (n > 0 && strstr(buf, "OK") != NULL)
            {
                httpdata_ok = true;
                break;
            }
        }

        ESP_LOGW(TAG, "HTTPDATA attempt %d failed (got: %.*s)",
                 retry + 1, n > 80 ? 80 : n, buf);
        gsm_cmd_ok("AT+HTTPTERM", GSM_TIMEOUT_SHORT);

        if (retry == 0)
        {
            /* Retry: close and re-open bearer, re-init HTTP */
            gsm_cmd_ok("AT+SAPBR=0,1", GSM_TIMEOUT_MEDIUM);
            vTaskDelay(MS2TICK(500));
            gsm_err_t gprs_err = gsm_cmd_ok("AT+SAPBR=1,1", GSM_TIMEOUT_GPRS);
            if (gprs_err != GSM_OK)
                return GSM_ERR_GPRS;

            err = gsm_http_init(url);
            if (err != GSM_OK)
                return err;

            const char *ct = content_type ? content_type : "application/x-www-form-urlencoded";
            char ct_cmd[128];
            snprintf(ct_cmd, sizeof(ct_cmd), "AT+HTTPPARA=\"CONTENT\",\"%s\"", ct);
            gsm_cmd_ok(ct_cmd, GSM_TIMEOUT_SHORT);
        }
        else
        {
            return GSM_ERR_HTTP;
        }
    }

    if (!httpdata_ok)
        return GSM_ERR_HTTP;

    /* Trigger POST (method = 1) — send directly to avoid gsm_cmd_ok
     * consuming the +HTTPACTION URC */
    gsm_uart_flush();
    if (gsm_uart_write_line("AT+HTTPACTION=1") != GSM_OK)
    {
        gsm_cmd_ok("AT+HTTPTERM", GSM_TIMEOUT_SHORT);
        return GSM_ERR_UART;
    }

    err = gsm_http_read_response(response);
    if (err != GSM_OK)
    {
        gsm_cmd_ok("AT+HTTPTERM", GSM_TIMEOUT_SHORT);
        return err;
    }

    gsm_cmd_ok("AT+HTTPTERM", GSM_TIMEOUT_SHORT);
    return GSM_OK;
}

/* ────────────────────────────────────────────────────────────────────────── *
 *  Utility
 * ────────────────────────────────────────────────────────────────────────── */

int gsm_rssi_to_dbm(int rssi)
{
    if (rssi == 99 || rssi < 0)
        return 0;
    /* SIM800 formula: dBm = -113 + 2 * RSSI */
    return -113 + (2 * rssi);
}

const char *gsm_err_to_str(gsm_err_t err)
{
    switch (err)
    {
    case GSM_OK:
        return "OK";
    case GSM_ERR_TIMEOUT:
        return "Timeout";
    case GSM_ERR_NO_RESP:
        return "No / unexpected response";
    case GSM_ERR_SIM:
        return "SIM error";
    case GSM_ERR_NOT_REG:
        return "Not registered";
    case GSM_ERR_GPRS:
        return "GPRS failure";
    case GSM_ERR_HTTP:
        return "HTTP failure";
    case GSM_ERR_CALL:
        return "Call failure";
    case GSM_ERR_SMS:
        return "SMS failure";
    case GSM_ERR_PARAM:
        return "Invalid parameter";
    case GSM_ERR_UART:
        return "UART error";
    default:
        return "Unknown error";
    }
}