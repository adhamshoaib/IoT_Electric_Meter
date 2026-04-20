#include "http_client.h"
#include "esp_crt_bundle.h"

static const char *TAG = "SMART_METER";

esp_err_t firebase_post(time_t ts, float energy_kwh)
{
    const char *url =
        "https://sem-rtdb-backend.onrender.com/reading";

    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"ts\": %lld, \"energy_kwh\": %.4f}",
             (long long)ts, energy_kwh);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 8000};

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "x-api-key", "sem-prod-x7k9m2p4q8");
    esp_http_client_set_post_field(client, payload, strlen(payload));

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK)
        return err;

    if (err == ESP_OK)
    {
        int code = esp_http_client_get_status_code(client);
        if (code != 200)
        {
            ESP_LOGE(TAG, "HTTP error: %d", code);
            err = ESP_FAIL;
        }
    }

    esp_http_client_cleanup(client);
    return err;
}