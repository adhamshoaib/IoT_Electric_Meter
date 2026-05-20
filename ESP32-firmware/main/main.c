#include "BL0939.h"
#include "uart_service.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BL0939_UART_PORT UART_NUM_2
#define BL0939_UART_TX_PIN GPIO_NUM_17
#define BL0939_UART_RX_PIN GPIO_NUM_16
#define BL0939_UART_BAUD_RATE 4800
#define BL0939_DEVICE_ADDRESS 0U /* A4..A1 strap: 0..15 (e.g. 5 -> read cmd 0x55) */

#define BL0939_DEFAULT_TIMEOUT_MS 1500U
#define BL0939_READ_PERIOD_MS 1000U

#define BL0939_VREF 1.218f
#define BL0939_VRMS_SCALE 79931.0f
#define BL0939_IRMS_SCALE 324004.0f
#define BL0939_ENERGY_REF 3304.0f
#define BL0939_CF_COUNT_SCALE 20000.0f
#define BL0939_ENERGY_DIVISOR (BL0939_ENERGY_REF * BL0939_CF_COUNT_SCALE)

#define MAINS_CALIB_VOLT 230.0f
#define VP_AT_MAINS_CALIB_VOLT 0.062f
#define DIVIDER_RATIO (MAINS_CALIB_VOLT / VP_AT_MAINS_CALIB_VOLT)
#define VAC_FINE_GAIN 0.956f
#define BL0939_VRMS_ZERO_OFFSET_RAW 2720U

#define IA_CAL_PIN_MV 12.0f
#define IA_CAL_RMS_A 0.4347f
#define IA_CAL_A_PER_MV (IA_CAL_RMS_A / IA_CAL_PIN_MV)
#define IA_PIN_FINE_GAIN 0.686f

#define IA_NOISE_FLOOR_A 0.003f
#define VP_NOISE_FLOOR_MV 0.2f
#define VAC_NOISE_FLOOR_V 0.5f

static const char *TAG = "MAIN";
static float s_total_energy_kwh = 0.0f;
static bool s_energy_counter_valid = false;
static int32_t s_prev_cfa_cnt = 0;
static int32_t s_prev_cfb_cnt = 0;

static uint32_t vrms_raw_apply_zero_offset(uint32_t v_rms_raw)
{
    if (v_rms_raw <= BL0939_VRMS_ZERO_OFFSET_RAW)
    {
        return 0U;
    }

    return v_rms_raw - BL0939_VRMS_ZERO_OFFSET_RAW;
}

static float vrms_raw_to_mains_v(uint32_t v_rms_raw)
{
    uint32_t corrected_raw = vrms_raw_apply_zero_offset(v_rms_raw);
    float vp_mv = ((float)corrected_raw * BL0939_VREF) / BL0939_VRMS_SCALE;
    float mains_v = ((vp_mv * DIVIDER_RATIO) / 1000.0f) * VAC_FINE_GAIN;

    return (mains_v < VAC_NOISE_FLOOR_V) ? 0.0f : mains_v;
}

static float irms_raw_to_ip_mv(uint32_t i_rms_raw)
{
    float ip_mv = (((float)i_rms_raw * BL0939_VREF) / BL0939_IRMS_SCALE) * IA_PIN_FINE_GAIN;
    return (ip_mv < VP_NOISE_FLOOR_MV) ? 0.0f : ip_mv;
}

static float irms_raw_to_amps(uint32_t i_rms_raw)
{
    float ip_mv = irms_raw_to_ip_mv(i_rms_raw);
    float amps = ip_mv * IA_CAL_A_PER_MV;

    return (amps < IA_NOISE_FLOOR_A) ? 0.0f : amps;
}

static float frame_energy_kwh(const bl0939_raw_data_t *raw)
{
    if (raw == NULL)
    {
        return 0.0f;
    }

    if (!s_energy_counter_valid)
    {
        s_prev_cfa_cnt = raw->cfa_cnt;
        s_prev_cfb_cnt = raw->cfb_cnt;
        s_energy_counter_valid = true;
        return 0.0f;
    }

    int32_t delta_cfa = raw->cfa_cnt - s_prev_cfa_cnt;
    int32_t delta_cfb = raw->cfb_cnt - s_prev_cfb_cnt;

    s_prev_cfa_cnt = raw->cfa_cnt;
    s_prev_cfb_cnt = raw->cfb_cnt;

    int32_t delta_total = delta_cfa + delta_cfb;
    if (delta_total <= 0)
    {
        return 0.0f;
    }

    return (float)delta_total / BL0939_ENERGY_DIVISOR;
}

static esp_err_t init_meter(uart_service_handle_t *out_uart)
{
    if (out_uart == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const uart_service_config_t uart_cfg = {
        .port = BL0939_UART_PORT,
        .tx_pin = BL0939_UART_TX_PIN,
        .rx_pin = BL0939_UART_RX_PIN,
        .baud_rate = BL0939_UART_BAUD_RATE,
        .rx_buffer_size = 256,
        .tx_buffer_size = 0,
    };

    esp_err_t ret = uart_service_init(&uart_cfg, out_uart);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = uart_service_set_stop_bits(*out_uart, UART_STOP_BITS_2);
    if (ret != ESP_OK)
    {
        (void)uart_service_deinit(out_uart);
        return ret;
    }

    const bl0939_config_t bl_cfg = {
        .uart = *out_uart,
        .device_address = BL0939_DEVICE_ADDRESS,
        .calibration = {
            .voltage_ref = BL0939_VRMS_SCALE,
            .current_ref = BL0939_IRMS_SCALE,
            .energy_ref = BL0939_ENERGY_DIVISOR,
        },
        .phase_compensation = {
            .corner_a = 0x0000,
            .corner_b = 0x0000,
        },
        .current_channel = BL0939_CURRENT_CHANNEL_SUM,
        .default_timeout_ms = BL0939_DEFAULT_TIMEOUT_MS,
        .auto_request_before_read = true,
    };

    ret = bl0939_init(&bl_cfg);
    if (ret != ESP_OK)
    {
        (void)uart_service_deinit(out_uart);
        return ret;
    }

    return ESP_OK;
}

static void log_measurements(const bl0939_raw_data_t *raw)
{
    if (raw == NULL)
    {
        return;
    }

    float voltage_v = vrms_raw_to_mains_v(raw->v_rms);
    float current_a = irms_raw_to_amps(raw->ia_rms);

    s_total_energy_kwh += frame_energy_kwh(raw);

    ESP_LOGI(TAG,
             "Voltage: %.2f V | Current: %.3f A | Energy: %.6f kWh",
             voltage_v,
             current_a,
             s_total_energy_kwh);
}

void app_main(void)
{
    uart_service_handle_t uart = NULL;

    esp_err_t ret = init_meter(&uart);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Meter initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "BL0939 test started (UART%d TX=%d RX=%d ADDR=%u)",
             BL0939_UART_PORT, BL0939_UART_TX_PIN, BL0939_UART_RX_PIN, (unsigned)BL0939_DEVICE_ADDRESS);

    while (true)
    {
        bl0939_raw_data_t raw;
        ret = bl0939_read_raw(&raw, BL0939_TIMEOUT_USE_DEFAULT);
        if (ret == ESP_OK)
        {
            log_measurements(&raw);
        }
        else
        {
            ESP_LOGW(TAG, "bl0939_read_raw failed: %s", esp_err_to_name(ret));
        }

        vTaskDelay(pdMS_TO_TICKS(BL0939_READ_PERIOD_MS));
    }
}
