#include "wifi_sta.h"
#include "esp_wifi_netif.h"

static const char *Tag = "WIFI_STA";

static esp_netif_t *S_wifi_netif = NULL;
static EventGroupHandle_t *s_wifi_event_group = NULL;
static wifi_netif_driver_t wifi_netif_driver = NULL;