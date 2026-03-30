#include "wifi.h"

#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"

#define WIFI_CONNECTED_BIT BIT0
#define RETRIES_PER_NETWORK 3 // attempts per SSID before moving to next

typedef struct
{
    const char *ssid;
    const char *password;
} wifi_network_t;

static const wifi_network_t wifi_list[] = {
    {"adham", "adham@9876"},
    {"Zeyad iPhone", "11111111"},
    {"ashoaib", "adham2205"},
};

#define WIFI_LIST_SIZE (sizeof(wifi_list) / sizeof(wifi_list[0]))

static const char *TAG = "WIFI";

static EventGroupHandle_t s_wifi_event_group;

static int s_retry_count = 0;
static int s_network_index = 0; // which SSID we're currently trying

// ── switch to the next known network ─────────────────────────────────────────

static void try_next_network(void)
{
    s_network_index = (s_network_index + 1) % WIFI_LIST_SIZE;
    s_retry_count = 0;

    wifi_config_t cfg = {0};
    strncpy((char *)cfg.sta.ssid, wifi_list[s_network_index].ssid,
            sizeof(cfg.sta.ssid) - 1);
    strncpy((char *)cfg.sta.password, wifi_list[s_network_index].password,
            sizeof(cfg.sta.password) - 1);
    cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_LOGI(TAG, "Switching to network: %s", wifi_list[s_network_index].ssid);
    esp_wifi_set_config(WIFI_IF_STA, &cfg);
    esp_wifi_connect();
}

// ── event handler ─────────────────────────────────────────────────────────────

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "STA started — connecting to %s", wifi_list[s_network_index].ssid);
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        // Clear connected bit whenever we drop
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        s_retry_count++;
        if (s_retry_count < RETRIES_PER_NETWORK)
        {
            ESP_LOGW(TAG, "[%s] retry %d/%d",
                     wifi_list[s_network_index].ssid, s_retry_count, RETRIES_PER_NETWORK);
            esp_wifi_connect();
        }
        else
        {
            ESP_LOGW(TAG, "[%s] failed — trying next network",
                     wifi_list[s_network_index].ssid);
            try_next_network();
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected to %s — IP: " IPSTR,
                 wifi_list[s_network_index].ssid,
                 IP2STR(&event->ip_info.ip));

        s_retry_count = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// ── public API ────────────────────────────────────────────────────────────────

void wifi_init(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

    // ── register handlers BEFORE start so no events are missed ──
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    // ── configure first network ──
    wifi_config_t wifi_cfg = {0};
    strncpy((char *)wifi_cfg.sta.ssid, wifi_list[0].ssid,
            sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *)wifi_cfg.sta.password, wifi_list[0].password,
            sizeof(wifi_cfg.sta.password) - 1);
    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start()); // fires WIFI_EVENT_STA_START → handler calls esp_wifi_connect()

    ESP_LOGI(TAG, "WiFi initialization finished.");
}

void wifi_wait_connected(void)
{
    xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE, // don't clear the bit
        pdFALSE,
        portMAX_DELAY);
}

bool wifi_is_connected(void)
{
    return (xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT) != 0;
}