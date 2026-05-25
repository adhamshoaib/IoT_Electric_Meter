#include "cloud_sync.h"

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "http_client.h"
#include "wifi_sta.h"
#include "energy_metering.h"

#include <stdatomic.h>
#include <time.h>

static const char *TAG = "CLOUD_SYNC";

static TaskHandle_t s_task_handle = NULL;
static SemaphoreHandle_t s_lock = NULL;
static atomic_bool s_initialized = false;
static atomic_bool s_time_synced = false;
static atomic_bool s_task_running = false;
static atomic_bool s_stop_requested = false;
static atomic_uint s_upload_count = 0;

static uint32_t s_upload_interval_ms = CLOUD_SYNC_DEFAULT_INTERVAL_MS;
static uint32_t s_retry_delay_ms = CLOUD_SYNC_DEFAULT_RETRY_MS;



static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "SNTP time synchronized");
    atomic_store(&s_time_synced, true);
}

static esp_err_t cloud_sync_obtain_time(void)
{
    if (atomic_load(&s_time_synced))
    {
        return ESP_OK;
    }

    if (!wifi_is_connected())
    {
        ESP_LOGW(TAG, "WiFi not connected, cannot sync time");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Initializing SNTP");

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, CONFIG_CLOUD_SYNC_NTP_SERVER_1);
    sntp_setservername(1, CONFIG_CLOUD_SYNC_NTP_SERVER_2);
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();

    const int retry_count = 10;
    for (int i = 0; i < retry_count && !atomic_load(&s_time_synced); i++)
    {
        ESP_LOGI(TAG, "Waiting for time sync... (%d/%d)", i + 1, retry_count);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    if (atomic_load(&s_time_synced))
    {
        time_t now;
        char time_buf[64];
        time(&now);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        strftime(time_buf, sizeof(time_buf), "%c", &timeinfo);
        ESP_LOGI(TAG, "Current time: %s", time_buf);
        return ESP_OK;
    }
    else
    {
        ESP_LOGW(TAG, "SNTP time sync failed - will retry later");
        return ESP_ERR_TIMEOUT;
    }
}

static time_t cloud_sync_get_timestamp(void)
{
    time_t now = 0;
    time(&now);

    if (now < 946684800)
    {
        atomic_store(&s_time_synced, false);
        return 0;
    }

    return now;
}

static void cloud_sync_task_entry(void *arg)
{
    (void)arg;

    ESP_LOGI(TAG, "Cloud sync task started");

    while (!atomic_load(&s_stop_requested))
    {
        const uint32_t sleep_ms = atomic_load(&s_stop_requested) ? 100 : s_upload_interval_ms;

        if (!wifi_is_connected())
        {
            ESP_LOGD(TAG, "WiFi not connected, skipping upload");
            vTaskDelay(pdMS_TO_TICKS(sleep_ms));
            continue;
        }

        if (!atomic_load(&s_time_synced))
        {
            ESP_LOGI(TAG, "Time not synchronized, attempting sync");
            (void)cloud_sync_obtain_time();
            if (!atomic_load(&s_time_synced))
            {
                vTaskDelay(pdMS_TO_TICKS(s_retry_delay_ms));
                continue;
            }
        }

        energy_metering_data_t data;
        esp_err_t ret = energy_metering_get_latest(&data);
        if (ret != ESP_OK)
        {
            ESP_LOGW(TAG, "Failed to get energy data: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(s_retry_delay_ms));
            continue;
        }

        time_t ts = cloud_sync_get_timestamp();
        if (ts == 0)
        {
            ESP_LOGW(TAG, "Invalid timestamp, skipping upload");
            vTaskDelay(pdMS_TO_TICKS(s_retry_delay_ms));
            continue;
        }

        ESP_LOGI(TAG, "Uploading: ts=%lld, energy=%.6f kWh",
                 (long long)ts, data.total_energy_kwh);

        ret = firebase_post(ts, data.total_energy_kwh);
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "Upload successful");
            atomic_fetch_add(&s_upload_count, 1);
            vTaskDelay(pdMS_TO_TICKS(sleep_ms));
        }
        else
        {
            ESP_LOGW(TAG, "Upload failed: %s, retrying in %d ms",
                     esp_err_to_name(ret), (int)s_retry_delay_ms);
            vTaskDelay(pdMS_TO_TICKS(s_retry_delay_ms));
        }
    }

    ESP_LOGI(TAG, "Cloud sync task stopping");
    s_task_handle = NULL;
    atomic_store(&s_task_running, false);
    vTaskDelete(NULL);
}

esp_err_t cloud_sync_init(void)
{
    if (atomic_load(&s_initialized))
    {
        return ESP_OK;
    }

    s_lock = xSemaphoreCreateMutex();
    if (s_lock == NULL)
    {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    s_upload_interval_ms = CONFIG_CLOUD_SYNC_UPLOAD_INTERVAL_MS;
    s_retry_delay_ms = CONFIG_CLOUD_SYNC_RETRY_DELAY_MS;

    atomic_store(&s_time_synced, false);
    atomic_store(&s_task_running, false);
    atomic_store(&s_stop_requested, false);
    atomic_store(&s_initialized, true);

    ESP_LOGI(TAG, "Cloud sync initialized (interval=%d ms, retry=%d ms)",
             (int)s_upload_interval_ms, (int)s_retry_delay_ms);

    return ESP_OK;
}

esp_err_t cloud_sync_start_task(void)
{
    if (!atomic_load(&s_initialized))
    {
        ESP_LOGE(TAG, "Cloud sync not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(s_lock, portMAX_DELAY) != pdTRUE)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_task_handle != NULL)
    {
        ESP_LOGW(TAG, "Cloud sync task already running");
        xSemaphoreGive(s_lock);
        return ESP_ERR_INVALID_STATE;
    }

    atomic_store(&s_stop_requested, false);
    atomic_store(&s_task_running, true);

    BaseType_t created = xTaskCreate(
        cloud_sync_task_entry,
        "cloud_sync",
        CONFIG_CLOUD_SYNC_TASK_STACK_SIZE,
        NULL,
        CONFIG_CLOUD_SYNC_TASK_PRIORITY,
        &s_task_handle);

    xSemaphoreGive(s_lock);

    if (created != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create cloud sync task");
        s_task_handle = NULL;
        atomic_store(&s_task_running, false);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Cloud sync task created");
    return ESP_OK;
}

esp_err_t cloud_sync_stop_task(void)
{
    if (!atomic_load(&s_initialized))
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(s_lock, portMAX_DELAY) != pdTRUE)
    {
        return ESP_ERR_INVALID_STATE;
    }

    TaskHandle_t task = s_task_handle;
    if (task == NULL)
    {
        xSemaphoreGive(s_lock);
        return ESP_ERR_INVALID_STATE;
    }

    atomic_store(&s_stop_requested, true);
    xSemaphoreGive(s_lock);

    ESP_LOGI(TAG, "Waiting for cloud sync task to stop...");

    const TickType_t timeout_ticks = pdMS_TO_TICKS(5000);
    const TickType_t start_ticks = xTaskGetTickCount();

    while (atomic_load(&s_task_running))
    {
        if ((xTaskGetTickCount() - start_ticks) >= timeout_ticks)
        {
            ESP_LOGW(TAG, "Cloud sync task stop timeout");
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGI(TAG, "Cloud sync task stopped");
    return ESP_OK;
}

bool cloud_sync_is_time_synced(void)
{
    return atomic_load(&s_time_synced);
}

bool cloud_sync_is_task_running(void)
{
    return atomic_load(&s_task_running);
}

uint32_t cloud_sync_get_upload_count(void)
{
    return atomic_load(&s_upload_count);
}

esp_err_t cloud_sync_deinit(void)
{
    (void)cloud_sync_stop_task();

    if (s_lock != NULL)
    {
        vSemaphoreDelete(s_lock);
        s_lock = NULL;
    }

    atomic_store(&s_initialized, false);
    ESP_LOGI(TAG, "Cloud sync deinitialized");

    return ESP_OK;
}
