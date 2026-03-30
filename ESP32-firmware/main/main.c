#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "adc_driver.h"

static const char *TAG = "ADC_TEST";

#define TEST_SAMPLE_RATE_HZ 20000
#define TEST_CYCLES_PER_WIN 4
#define TEST_SAMPLES_PER_WIN (TEST_SAMPLE_RATE_HZ / 50 * TEST_CYCLES_PER_WIN)
#define TEST_READ_WINDOWS 5
#define TEST_TIMEOUT_MS 200
#define ADC_12BIT_MAX 4095

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

static void assert_eq(const char *label, esp_err_t got, esp_err_t want)
{
    if (got == want)
        ESP_LOGI(TAG, "  PASS  %-40s  got %s", label, esp_err_to_name(got));
    else
        ESP_LOGE(TAG, "  FAIL  %-40s  want %s, got %s",
                 label, esp_err_to_name(want), esp_err_to_name(got));
}

/* ------------------------------------------------------------------ */
/* Test body                                                            */
/* ------------------------------------------------------------------ */

static void run_tests(void)
{
    esp_err_t ret;

    /* --- 1. Baseline init ----------------------------------------- */
    ESP_LOGI(TAG, "--- init");
    ret = adc_driver_init();
    assert_eq("adc_driver_init()", ret, ESP_OK);

    /* --- 2. Guard: double-init must be rejected -------------------- */
    ESP_LOGI(TAG, "--- state guards (before start)");
    ret = adc_driver_init();
    assert_eq("double init → INVALID_STATE", ret, ESP_ERR_INVALID_STATE);

    ret = adc_driver_stop();
    assert_eq("stop() before start → INVALID_STATE", ret, ESP_ERR_INVALID_STATE);

    /* --- 3. Guard: read() while not running ----------------------- */
    {
        adc_sample_pair_t *buf = malloc(TEST_SAMPLES_PER_WIN * sizeof(adc_sample_pair_t));
        if (buf == NULL)
        {
            ESP_LOGE(TAG, "malloc failed, aborting test");
            return;
        }

        size_t count = 0;
        ret = adc_driver_read(buf, TEST_SAMPLES_PER_WIN, &count, TEST_TIMEOUT_MS);
        assert_eq("read() before start → INVALID_STATE", ret, ESP_ERR_INVALID_STATE);

        /* --- 4. Guard: NULL / zero-count args --------------------- */
        ret = adc_driver_start();
        assert_eq("adc_driver_start()", ret, ESP_OK);

        ret = adc_driver_read(NULL, TEST_SAMPLES_PER_WIN, &count, TEST_TIMEOUT_MS);
        assert_eq("read(NULL buf) → INVALID_ARG", ret, ESP_ERR_INVALID_ARG);

        ret = adc_driver_read(buf, 0, &count, TEST_TIMEOUT_MS);
        assert_eq("read(max_count=0) → INVALID_ARG", ret, ESP_ERR_INVALID_ARG);

        ret = adc_driver_read(buf, TEST_SAMPLES_PER_WIN, NULL, TEST_TIMEOUT_MS);
        assert_eq("read(NULL out_count) → INVALID_ARG", ret, ESP_ERR_INVALID_ARG);

        /* --- 5. Guard: double-start ------------------------------- */
        ret = adc_driver_start();
        assert_eq("double start → INVALID_STATE", ret, ESP_ERR_INVALID_STATE);

        /* --- 6. Read loop ----------------------------------------- */
        ESP_LOGI(TAG, "--- read loop (%d windows)", TEST_READ_WINDOWS);

        for (int w = 0; w < TEST_READ_WINDOWS; w++)
        {
            memset(buf, 0, TEST_SAMPLES_PER_WIN * sizeof(adc_sample_pair_t));
            count = 0;

            ret = adc_driver_read(buf, TEST_SAMPLES_PER_WIN, &count, TEST_TIMEOUT_MS);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "  FAIL  window %d read returned %s", w, esp_err_to_name(ret));
                continue;
            }

            if (count == 0)
            {
                ESP_LOGE(TAG, "  FAIL  window %d: count=0", w);
                continue;
            }

            bool range_ok = true;
            for (size_t s = 0; s < count; s++)
            {
                if (buf[s].v_raw > ADC_12BIT_MAX || buf[s].i_raw > ADC_12BIT_MAX)
                {
                    ESP_LOGE(TAG, "  FAIL  window %d sample %zu out of range: v=%u i=%u",
                             w, s, buf[s].v_raw, buf[s].i_raw);
                    range_ok = false;
                }
            }

            if (range_ok)
                ESP_LOGI(TAG, "  PASS  window %d: %zu pairs, first v_raw=%u i_raw=%u",
                         w, count, buf[0].v_raw, buf[0].i_raw);
        }

        free(buf);
    }

    /* --- 7. Live monitor ------------------------------------------ */
    ESP_LOGI(TAG, "--- live monitor (1 s averages, reset to stop)");

    adc_sample_pair_t *mon_buf = malloc(TEST_SAMPLES_PER_WIN * sizeof(adc_sample_pair_t));
    if (mon_buf == NULL)
    {
        ESP_LOGE(TAG, "malloc failed for monitor buffer");
        goto stop;
    }

    while (1)
    {
        uint64_t v_sum = 0;
        uint64_t i_sum = 0;
        size_t total_pairs = 0;

        TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(1000);

        while (xTaskGetTickCount() < deadline)
        {
            size_t count = 0;
            ret = adc_driver_read(mon_buf, TEST_SAMPLES_PER_WIN, &count, TEST_TIMEOUT_MS);
            if (ret != ESP_OK)
            {
                ESP_LOGW(TAG, "read error: %s", esp_err_to_name(ret));
                break;
            }

            for (size_t s = 0; s < count; s++)
            {
                v_sum += mon_buf[s].v_raw;
                i_sum += mon_buf[s].i_raw;
            }
            total_pairs += count;
        }

        if (total_pairs > 0)
        {
            uint32_t v_avg = (uint32_t)(v_sum / total_pairs);
            uint32_t i_avg = (uint32_t)(i_sum / total_pairs);
            uint32_t v_mv = (v_avg * 3300) / 4095;
            uint32_t i_mv = (i_avg * 3300) / 4095;

            ESP_LOGI(TAG, "pairs=%-6zu  v_avg=%-4" PRIu32 " (%4" PRIu32 " mV)  i_avg=%-4" PRIu32 " (%4" PRIu32 " mV)",
                     total_pairs, v_avg, v_mv, i_avg, i_mv);
        }
        else
        {
            ESP_LOGW(TAG, "no samples collected this second");
        }
    }

    free(mon_buf);

stop:
    /* --- 8. Stop -------------------------------------------------- */
    ESP_LOGI(TAG, "--- stop");
    ret = adc_driver_stop();
    assert_eq("adc_driver_stop()", ret, ESP_OK);

    ret = adc_driver_stop();
    assert_eq("double stop → INVALID_STATE", ret, ESP_ERR_INVALID_STATE);

    /* --- 9. Deinit ------------------------------------------------ */
    ESP_LOGI(TAG, "--- deinit");
    ret = adc_driver_deinit();
    assert_eq("adc_driver_deinit()", ret, ESP_OK);

    ret = adc_driver_deinit();
    assert_eq("double deinit → INVALID_STATE", ret, ESP_ERR_INVALID_STATE);

    ESP_LOGI(TAG, "--- done");
}

/* ------------------------------------------------------------------ */

void app_main(void)
{
    ESP_LOGI(TAG, "ADC driver test start");
    run_tests();
    ESP_LOGI(TAG, "ADC driver test end — check for FAIL lines above");
}