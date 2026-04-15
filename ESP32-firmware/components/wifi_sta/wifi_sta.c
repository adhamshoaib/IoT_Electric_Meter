#include "wifi_sta.h"
#include "esp_wifi_netif.h"
#include "esp_log.h"

static const char *Tag = "WIFI_STA";

static esp_netif_t *S_wifi_netif = NULL;
static EventGroupHandle_t *s_wifi_event_group = NULL;
static wifi_netif_driver_t wifi_netif_driver = NULL;

static void on_wifi_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static void on_ip_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static void wifi_start(void *esp_netif, esp_event_base_t base, int32_t event_id, void *data);

/////////////////////////////////////////////////////////////////////////////////////////////////

static void on_wifi_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
        // wifi started
    case WIFI_EVENT_STA_START:
        if (S_wifi_netif != NULL)
        {
            wifi_start(S_wifi_netif, event_base, event_id, event_data);
        }
        break;

    // wifi stopped
    case WIFI_EVENT_STA_STOP:
        if (S_wifi_netif != NULL)
        {
            esp_netif_action_stop(S_wifi_netif, event_base, event_id, event_data);
        }
        break;

    case WIFI_EVENT_STA_CONNECTED:
        if (S_wifi_netif == NULL)
        {
            ESP_LOGE(TAG, "wifi not started");
            return;
        }
    }

    // Print AP information
    wifi_event_sta_connected_t *event_sta_connected = (wifi_event_sta_connected_t *)event_data;

    ESP_LOGI(TAG, "Connected to AP");
    ESP_LOGI(TAG, "  SSID: %s", (char *)event_sta_connected->ssid);
    ESP_LOGI(TAG, "  Channel: %d", event_sta_connected->channel);
    ESP_LOGI(TAG, "  Auth mode: %d", event_sta_connected->authmode);
    ESP_LOGI(TAG, "  AID: %d", event_sta_connected->aid);
}
