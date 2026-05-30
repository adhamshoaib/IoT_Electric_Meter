#include "gsm_ppp.h"
#include "gsm_driver.h"

#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/dns.h"
#include "netif/ppp/pppapi.h"
#include "netif/ppp/pppos.h"

static const char *TAG = "GSM_PPP";

static ppp_pcb *s_ppp = NULL;
static TaskHandle_t s_uart_rx_task = NULL;
static volatile bool s_ppp_connected = false;
static struct netif s_ppp_netif;

static u32_t pppos_output_cb(ppp_pcb *pcb, const void *data, u32_t len, void *ctx)
{
    int written = uart_write_bytes(GSM_UART_PORT, data, len);
    uart_wait_tx_done(GSM_UART_PORT, pdMS_TO_TICKS(1000));
    return (written >= 0) ? (u32_t)written : 0;
}

static void pppos_link_status_cb(ppp_pcb *pcb, int err_code, void *ctx)
{
    switch (err_code)
    {
    case PPPERR_NONE:
        ESP_LOGI(TAG, "PPP connected");
        s_ppp_connected = true;
        break;
    case PPPERR_CONNECT:
        ESP_LOGI(TAG, "PPP disconnected");
        s_ppp_connected = false;
        break;
    default:
        ESP_LOGW(TAG, "PPP error: %d", err_code);
        s_ppp_connected = false;
        break;
    }
}

static void uart_rx_task(void *arg)
{
    uint8_t buf[256];
    ESP_LOGI(TAG, "UART RX task started");

    while (s_ppp != NULL)
    {
        int len = uart_read_bytes(GSM_UART_PORT, buf, sizeof(buf), pdMS_TO_TICKS(100));
        if (len > 0 && s_ppp != NULL)
        {
            pppos_input(s_ppp, (const void *)buf, len);
        }
    }

    ESP_LOGI(TAG, "UART RX task stopped");
    vTaskDelete(NULL);
}

esp_err_t gsm_ppp_start(const gsm_gprs_config_t *config, uint32_t timeout_ms)
{
    if (!config)
        return ESP_ERR_INVALID_ARG;

    if (s_ppp != NULL)
        return ESP_ERR_INVALID_STATE;

    s_ppp_connected = false;

    /* Deactivate any existing PDP context before redefining */
    gsm_cmd_ok("AT+CGACT=0,1", GSM_TIMEOUT_LONG);

    /* Define PDP context */
    char cmd[160];
    snprintf(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"",
             config->apn ? config->apn : "internet");
    if (gsm_cmd_ok(cmd, GSM_TIMEOUT_SHORT) != GSM_OK)
    {
        ESP_LOGE(TAG, "CGDCONT failed");
        return ESP_FAIL;
    }

    /* Enter PPP data mode — modem sends CONNECT then raw PPP frames */
    char buf[64];
    gsm_err_t ret = gsm_send_at("AT+CGDATA=\"PPP\",1", buf, sizeof(buf), 15000);
    if (ret != GSM_OK || !strstr(buf, "CONNECT"))
    {
        ESP_LOGE(TAG, "CGDATA failed: %s / %.*s", gsm_err_to_str(ret), 40, buf);
        return ESP_FAIL;
    }

    /* Drain any extra bytes after CONNECT before PPP starts */
    vTaskDelay(pdMS_TO_TICKS(200));
    uart_flush_input(GSM_UART_PORT);

    /* Create lwIP PPP PCB — MUST supply a valid netif (lwIP no longer embeds one) */
    s_ppp = pppapi_pppos_create(&s_ppp_netif, pppos_output_cb, pppos_link_status_cb, NULL);
    if (s_ppp == NULL)
    {
        ESP_LOGE(TAG, "pppos_create failed — restoring AT command mode");
        gsm_ppp_stop();
        return ESP_FAIL;
    }

    pppapi_set_default(s_ppp);

    /* Set PAP/CHAP credentials if provided */
    if (config->user || config->pass)
    {
        s_ppp->settings.user = config->user;
        s_ppp->settings.passwd = config->pass;
    }

    /* Start UART RX task */
    BaseType_t task_ok = xTaskCreatePinnedToCore(
        uart_rx_task, "ppp_rx", 2048, NULL, 10, &s_uart_rx_task, 1);
    if (task_ok != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create UART RX task");
        gsm_ppp_stop();
        return ESP_ERR_NO_MEM;
    }

    /* Start LCP negotiation */
    esp_err_t conn_err = pppapi_connect(s_ppp, 0);
    if (conn_err != ESP_OK)
    {
        ESP_LOGE(TAG, "pppapi_connect failed: %d", conn_err);
        gsm_ppp_stop();
        return ESP_FAIL;
    }

    /* Wait for IP connectivity */
    uint32_t waited = 0;
    while (!s_ppp_connected && waited < timeout_ms)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        waited += 100;
    }

    if (!s_ppp_connected)
    {
        ESP_LOGE(TAG, "PPP timeout after %u ms", timeout_ms);
        gsm_ppp_stop();
        return ESP_ERR_TIMEOUT;
    }

    struct netif *ppp_netif = s_ppp->netif;
    if (ppp_netif)
    {
        const ip4_addr_t *ip4 = ip_2_ip4(&ppp_netif->ip_addr);
        const ip4_addr_t *gw4 = ip_2_ip4(&ppp_netif->gw);
        ESP_LOGI(TAG, "PPP IP: %d.%d.%d.%d, GW: %d.%d.%d.%d",
                 ip4_addr1_16(ip4), ip4_addr2_16(ip4),
                 ip4_addr3_16(ip4), ip4_addr4_16(ip4),
                 ip4_addr1_16(gw4), ip4_addr2_16(gw4),
                 ip4_addr3_16(gw4), ip4_addr4_16(gw4));
    }

    /* Log and verify DNS servers negotiated via IPCP */
    for (int i = 0; i < DNS_MAX_SERVERS; i++)
    {
        const ip_addr_t *dns = dns_getserver(i);
        if (dns && !ip_addr_isany(dns))
        {
            ESP_LOGI(TAG, "DNS%d: %d.%d.%d.%d", i,
                     ip4_addr1_16(ip_2_ip4(dns)),
                     ip4_addr2_16(ip_2_ip4(dns)),
                     ip4_addr3_16(ip_2_ip4(dns)),
                     ip4_addr4_16(ip_2_ip4(dns)));
        }
    }

    /* Set fallback DNS — carrier rarely provides working external DNS */
    {
        ip_addr_t dns0, dns1;
        IP4_ADDR(ip_2_ip4(&dns0), 8, 8, 8, 8);
        IP4_ADDR(ip_2_ip4(&dns1), 8, 8, 4, 4);
        dns_setserver(0, &dns0);
        dns_setserver(1, &dns1);
        ESP_LOGI(TAG, "DNS fallback: 8.8.8.8 / 8.8.4.4");
    }

    return ESP_OK;
}

esp_err_t gsm_ppp_stop(void)
{
    /* Close PPP — signals uart_rx_task to exit via s_ppp = NULL */
    if (s_ppp)
    {
        ppp_pcb *pcb = s_ppp;
        s_ppp = NULL;
        pppapi_close(pcb, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
        pppapi_free(pcb);
    }

    s_ppp_connected = false;

    /* Wait for uart_rx_task to notice s_ppp == NULL and exit */
    if (s_uart_rx_task)
    {
        vTaskDelay(pdMS_TO_TICKS(200));
        s_uart_rx_task = NULL;
    }

    /* Return to AT command mode via +++ escape (SIM800 requires ≥1s guard time) */
    uart_wait_tx_done(GSM_UART_PORT, pdMS_TO_TICKS(1000));
    vTaskDelay(pdMS_TO_TICKS(1500));

    uart_flush_input(GSM_UART_PORT);
    uart_write_bytes(GSM_UART_PORT, (const uint8_t *)"+++", 3);
    uart_wait_tx_done(GSM_UART_PORT, pdMS_TO_TICKS(1000));
    vTaskDelay(pdMS_TO_TICKS(1500));

    gsm_cmd_ok("AT", GSM_TIMEOUT_SHORT);

    ESP_LOGI(TAG, "PPP stopped");
    return ESP_OK;
}

bool gsm_ppp_is_connected(void)
{
    return s_ppp_connected;
}
