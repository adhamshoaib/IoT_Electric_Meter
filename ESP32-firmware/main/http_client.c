#include "http_client.h"
static const char *TAG = "SMART_METER";

 esp_err_t firebase_put_last(time_t ts, float energy_kwh)
{
    const char *url =
"https://test2-8c525-default-rtdb.europe-west1.firebasedatabase.app/test/last.json";

    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"ts\": %lld, \"energy_kwh\": %.4f}",
             (long long)ts, energy_kwh);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_PUT,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 8000
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, payload, strlen(payload));

    esp_err_t err = esp_http_client_perform(client);
    if (err!=ESP_OK)
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