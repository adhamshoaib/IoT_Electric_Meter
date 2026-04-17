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
#define READ_BUF_LEN 1024       // keep above max pairs/read to avoid truncation bias
#define READ_TIMEOUT_MS 200
#define WARMUP_MS 1000

static adc_sample_pair_t s_buf[READ_BUF_LEN]; // static, not stack

static const char *TAG = "ADC_NOISE_TEST";
typedef struct
{
    uint64_t n;
    double mean;
    double m2;
    int32_t min;
    int32_t max;
} ch_stats_t;
static void stats_reset(ch_stats_t *s)
{
    s->n = 0;
    s->mean = 0.0;
    s->m2 = 0.0;
    s->min = INT32_MAX;
    s->max = INT32_MIN;
}
static void stats_add(ch_stats_t *s, int32_t x)
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
static int32_t stats_p2p(const ch_stats_t *s)
{
    if (s->n == 0)
        return 0;
    return s->max - s->min;
}
static void print_stats(const char *label, const ch_stats_t *s)
{
    ESP_LOGI(TAG, "%s n=%" PRIu64 " mean=%.2f std=%.2f p2p=%" PRIi32 " min=%" PRIi32 " max=%" PRIi32,
             label, s->n, s->mean, stats_stddev(s), stats_p2p(s), s->min, s->max);
}
void app_main(void)
{
    size_t count = 0;
    ch_stats_t v_total, i_total, v_sec, i_sec;
    uint64_t read_timeout_total = 0;
    uint64_t invalid_frame_total = 0;
    uint64_t read_timeout_sec = 0;
    uint64_t invalid_frame_sec = 0;
    uint64_t pairs_total = 0;
    uint64_t pairs_sec = 0;
    uint64_t full_buffer_reads_total = 0;
    uint64_t full_buffer_reads_sec = 0;

    stats_reset(&v_total);
    stats_reset(&i_total);
    stats_reset(&v_sec);
    stats_reset(&i_sec);
    esp_log_level_set("ADC_DRIVER", ESP_LOG_ERROR); // suppress warning flood
    ESP_ERROR_CHECK(adc_driver_init());
    ESP_ERROR_CHECK(adc_driver_start());
    int64_t warmup_start = esp_timer_get_time();
    int64_t warmup_end = warmup_start + (int64_t)WARMUP_MS * 1000;
    while (esp_timer_get_time() < warmup_end)
    {
        esp_err_t ret = adc_driver_read(s_buf, READ_BUF_LEN, &count, READ_TIMEOUT_MS);
        if (ret != ESP_OK && ret != ESP_ERR_TIMEOUT && ret != ESP_ERR_INVALID_RESPONSE)
        {
            ESP_LOGW(TAG, "Warmup read: %s", esp_err_to_name(ret));
        }
    }
    int64_t test_start = esp_timer_get_time();
    int64_t test_end = test_start + (int64_t)TEST_DURATION_S * 1000000;
    int64_t next_report = test_start + (int64_t)REPORT_INTERVAL_MS * 1000;
    ESP_LOGI(TAG, "Starting calibrated noise capture for %d s...", TEST_DURATION_S);
    while (esp_timer_get_time() < test_end)
    {
        esp_err_t ret = adc_driver_read(s_buf, READ_BUF_LEN, &count, READ_TIMEOUT_MS);
        if (ret == ESP_ERR_TIMEOUT)
        {
            read_timeout_total++;
            read_timeout_sec++;
        }
        else if (ret == ESP_ERR_INVALID_RESPONSE)
        {
            invalid_frame_total++;
            invalid_frame_sec++;
        }
        else if (ret != ESP_OK)
        {
            ESP_LOGW(TAG, "ADC read error: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        else
        {
            pairs_total += count;
            pairs_sec += count;
            if (count == READ_BUF_LEN)
            {
                full_buffer_reads_total++;
                full_buffer_reads_sec++;
            }

            for (size_t k = 0; k < count; k++)
            {
                stats_add(&v_total, s_buf[k].v_mv);
                stats_add(&v_sec, s_buf[k].v_mv);
                stats_add(&i_total, s_buf[k].i_mv);
                stats_add(&i_sec, s_buf[k].i_mv);
            }
        }

        int64_t now = esp_timer_get_time();
        if (now >= next_report)
        {
            int64_t elapsed_s = (now - test_start) / 1000000;
            ESP_LOGI(TAG, "t=%" PRIi64 "s", elapsed_s);
            print_stats("V(sec,mV)", &v_sec);
            print_stats("I(sec,mV)", &i_sec);
            ESP_LOGI(TAG, "Throughput: pairs=%" PRIu64 " (%.1f pairs/s)",
                     pairs_sec, (double)pairs_sec * 1000.0 / (double)REPORT_INTERVAL_MS);
            if (full_buffer_reads_sec > 0)
            {
                ESP_LOGW(TAG, "Read buffer filled %" PRIu64 " times this interval; results may be truncated",
                         full_buffer_reads_sec);
            }
            if (read_timeout_sec > 0 || invalid_frame_sec > 0)
            {
                ESP_LOGW(TAG, "Read retries this interval: timeout=%" PRIu64 " incomplete_pairs=%" PRIu64,
                         read_timeout_sec, invalid_frame_sec);
            }

            stats_reset(&v_sec);
            stats_reset(&i_sec);
            pairs_sec = 0;
            full_buffer_reads_sec = 0;
            read_timeout_sec = 0;
            invalid_frame_sec = 0;
            next_report += (int64_t)REPORT_INTERVAL_MS * 1000;
        }
    }
    ESP_LOGI(TAG, "Final results:");
    print_stats("V(total,mV)", &v_total);
    print_stats("I(total,mV)", &i_total);
    double total_duration_s = (double)(esp_timer_get_time() - test_start) / 1000000.0;
    ESP_LOGI(TAG, "Captured pairs=%" PRIu64 " over %.2f s (%.1f pairs/s)",
             pairs_total,
             total_duration_s,
             (total_duration_s > 0.0) ? ((double)pairs_total / total_duration_s) : 0.0);
    if (full_buffer_reads_total > 0)
    {
        ESP_LOGW(TAG, "Read buffer filled %" PRIu64 " times total; increase READ_BUF_LEN if this persists",
                 full_buffer_reads_total);
    }
    if (read_timeout_total > 0 || invalid_frame_total > 0)
    {
        ESP_LOGW(TAG, "Total read retries: timeout=%" PRIu64 " incomplete_pairs=%" PRIu64,
                 read_timeout_total, invalid_frame_total);
    }

    ESP_ERROR_CHECK(adc_driver_stop());
    ESP_ERROR_CHECK(adc_driver_deinit());
    ESP_LOGI(TAG, "Done.");
    vTaskSuspend(NULL);
}
