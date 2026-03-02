#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/apps/sntp.h"
#include "esp_netif.h"
#include "wifi.h"
#include "http_client.h"

static const char *TAG = "MAIN";

float energy_kwh = 0.0;
float power_kw = 1.0; // Simulated 1 kW load

void obtain_time()
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    time_t now = 0;
    struct tm timeinfo = {0};

    while (timeinfo.tm_year < (2020 - 1900)) {
        time(&now);
        localtime_r(&now, &timeinfo);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Time obtained: %s", asctime(&timeinfo));
}

void app_main(void)
{
    // NVS init
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // WiFi init and wait for connection
    wifi_init();
    wifi_wait_connected();

    ESP_LOGI(TAG, "WiFi connected, starting SNTP...");
    obtain_time();

    // Main loop
    while (1)
    {
        // Energy accumulation in kWh
        float hours = 5.0 / 3600.0; // 5 seconds in hours
        energy_kwh += power_kw * hours;

        // Get Unix timestamp
        time_t now;
        time(&now);

        // Print JSON locally
        printf("{\"ts\": %lld, \"energy_kwh\": %.4f}\n", (long long) now, energy_kwh);

        // Upload to Firebase if WiFi is connected
        if (wifi_is_connected()) {
            firebase_put_last(now, energy_kwh);
        } else {
            ESP_LOGW(TAG, "WiFi not connected, skipping Firebase upload");
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS); // 5 seconds
    }
}