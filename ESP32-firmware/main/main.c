#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include <limits.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "adc_driver.h"

#define TEST_DURATION_S 120
#define REPORT_INTERVAL_MS 5000 // less spam
#define READ_BUF_LEN 256        // smaller, safer
#define READ_TIMEOUT_MS 200
#define WARMUP_MS 1000

static adc_sample_pair_t s_buf[READ_BUF_LEN]; // static, not stack

static const char *TAG = "ADC_NOISE_TEST";
typedef struct
{
    uint64_t n;
    double mean;
    double m2;
    uint16_t min;
    uint16_t max;
} ch_stats_t;
static void stats_reset(ch_stats_t *s)
{
    s->n = 0;
    s->mean = 0.0;
    s->m2 = 0.0;
    s->min = UINT16_MAX;
    s->max = 0;
}
static void stats_add(ch_stats_t *s, uint16_t x)
{
    s->n++;
    if (x < s->min)
        s->min = x;
    if (x > s->max)
        s->max = x;
    double dx = (double)x - s->mean;
    s->mean += dx / (double)s->n;
    double dx2 = (double)x - s->mean;
    s->m2 += dx * dx2;
}
static double stats_stddev(const ch_stats_t *s)
{
    if (s->n < 2)
        return 0.0;
    return sqrt(s->m2 / (double)(s->n - 1));
}
static uint16_t stats_p2p(const ch_stats_t *s)
{
    if (s->n == 0)
        return 0;
    return (uint16_t)(s->max - s->min);
}
static void print_stats(const char *label, const ch_stats_t *s)
{
    ESP_LOGI(TAG, "%s n=%" PRIu64 " mean=%.2f std=%.2f p2p=%u min=%u max=%u",
             label, s->n, s->mean, stats_stddev(s), stats_p2p(s), s->min, s->max);
}
void app_main(void)
{
    size_t count = 0;
    ch_stats_t v_total, i_total, v_sec, i_sec;
    stats_reset(&v_total);
    stats_reset(&i_total);
    stats_reset(&v_sec);
    stats_reset(&i_sec);
    esp_log_level_set("ADC_DRIVER", ESP_LOG_ERROR); // suppress warning flood
    ESP_ERROR_CHECK(adc_driver_init());
    ESP_ERROR_CHECK(adc_driver_start());
    int64_t t0 = esp_timer_get_time();
    int64_t warmup_end = t0 + (int64_t)WARMUP_MS * 1000;
    int64_t test_end = t0 + (int64_t)TEST_DURATION_S * 1000000;
    int64_t next_report = t0 + (int64_t)REPORT_INTERVAL_MS * 1000;
    while (esp_timer_get_time() < warmup_end)
    {
        esp_err_t ret = adc_driver_read(s_buf, READ_BUF_LEN, &count, READ_TIMEOUT_MS);
        if (ret != ESP_OK && ret != ESP_ERR_TIMEOUT && ret != ESP_ERR_INVALID_RESPONSE)
        {
            ESP_LOGW(TAG, "Warmup read: %s", esp_err_to_name(ret));
        }
    }
    ESP_LOGI(TAG, "Starting noise capture for %d s...", TEST_DURATION_S);
    while (esp_timer_get_time() < test_end)
    {
        esp_err_t ret = adc_driver_read(s_buf, READ_BUF_LEN, &count, READ_TIMEOUT_MS);
        if (ret == ESP_ERR_TIMEOUT || ret == ESP_ERR_INVALID_RESPONSE)
        {
            continue; // non-fatal
        }
        if (ret != ESP_OK)
        {
            ESP_LOGW(TAG, "ADC read error: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(10));
            continue; // do NOT reset
        }
        for (size_t k = 0; k < count; k++)
        {
            stats_add(&v_total, s_buf[k].v_raw);
            stats_add(&i_total, s_buf[k].i_raw);
            stats_add(&v_sec, s_buf[k].v_raw);
            stats_add(&i_sec, s_buf[k].i_raw);
        }
        int64_t now = esp_timer_get_time();
        if (now >= next_report)
        {
            int64_t elapsed_s = (now - t0) / 1000000;
            ESP_LOGI(TAG, "t=%" PRIi64 "s", elapsed_s);
            print_stats("V(sec)", &v_sec);
            print_stats("I(sec)", &i_sec);
            stats_reset(&v_sec);
            stats_reset(&i_sec);
            next_report += (int64_t)REPORT_INTERVAL_MS * 1000;
        }
    }
    ESP_LOGI(TAG, "Final results:");
    print_stats("V(total)", &v_total);
    print_stats("I(total)", &i_total);
    ESP_ERROR_CHECK(adc_driver_stop());
    ESP_ERROR_CHECK(adc_driver_deinit());
    ESP_LOGI(TAG, "Done.");
    vTaskSuspend(NULL);
}
