#include "wifi_sta.h"
#include "esp_wifi_netif.h"

static const char *Tag = "WIFI_STA";

static esp_netif_t *S_wifi_netif = NULL;
static EventGroupHandle_t *s_wifi_event_group = NULL;
static wifi_netif_driver_t wifi_netif_driver = NULL;

static void on_wifi_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static void on_ip_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static void on_wifi_start(void *esp_netif, esp_event_base_t base, int32_t event_id, void *data);