#include "BL0939.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BL0939_CMD_READ_REQUEST 0x55U
#define BL0939_CMD_READ_FULL_PACKET 0xAAU

#define BL0939_CMD_WRITE_REQUEST 0xA5U
#define BL0939_WRITE_PACKET_SIZE_BYTES 6U

#define FRAME_OFFSET_IA_RMS 4U
#define FRAME_OFFSET_IB_RMS 7U
#define FRAME_OFFSET_V_RMS 10U
#define FRAME_OFFSET_CFA_CNT 22U
#define FRAME_OFFSET_CFB_CNT 25U
#define FRAME_CHECKSUM_INDEX (BL0939_FRAME_SIZE_BYTES - 1U)

typedef struct
{
    uart_service_handle_t uart;
    bl0939_calibration_t calibration;
    bl0939_phase_compensation_t phase_compensation;
    bl0939_current_channel_t current_channel;
    uint32_t default_timeout_ms;
    bool auto_request_before_read;
    bool initialized;
} bl0939_state_t;

static bl0939_state_t s_bl0939 = { .initialized = false };

static bool calibration_is_valid(const bl0939_calibration_t *c)
{
    return c != NULL && c->voltage_ref > 0.0f && c->current_ref > 0.0f && c->energy_ref > 0.0f;
}

static uint32_t resolve_timeout(uint32_t timeout_ms)
{
    return (timeout_ms == BL0939_TIMEOUT_USE_DEFAULT) ? s_bl0939.default_timeout_ms : timeout_ms;
}

static uint32_t decode_u24_le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
}

static int32_t decode_s24_le(const uint8_t *p)
{
    uint32_t raw = decode_u24_le(p);
    if ((raw & 0x800000U) != 0U)
    {
        raw |= 0xFF000000U;
    }
    return (int32_t)raw;
}

static esp_err_t uart_send_with_timeout(const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    (void)timeout_ms;
    return uart_service_send(s_bl0939.uart, data, len);
}

static esp_err_t write_register(uint8_t reg, uint16_t value, uint32_t timeout_ms)
{
    uint8_t packet[BL0939_WRITE_PACKET_SIZE_BYTES];
    packet[0] = BL0939_CMD_WRITE_REQUEST;
    packet[1] = reg;
    packet[2] = (uint8_t)(value & 0xFFU);
    packet[3] = (uint8_t)((value >> 8) & 0xFFU);
    packet[4] = 0x00U;

    uint8_t checksum = 0U;
    for (size_t i = 0; i < BL0939_WRITE_PACKET_SIZE_BYTES - 1U; i++)
    {
        checksum = (uint8_t)(checksum + packet[i]);
    }
    packet[5] = (uint8_t)(0xFFU ^ checksum);

    return uart_send_with_timeout(packet, sizeof(packet), timeout_ms);
}

static bool frame_is_valid(const uint8_t *frame)
{
    if (frame[0] != BL0939_FRAME_HEADER_VALUE)
    {
        return false;
    }

    uint8_t checksum = BL0939_CMD_READ_REQUEST;
    for (size_t i = 0; i < FRAME_CHECKSUM_INDEX; i++)
    {
        checksum = (uint8_t)(checksum + frame[i]);
    }
    checksum = (uint8_t)(0xFFU ^ checksum);

    return checksum == frame[FRAME_CHECKSUM_INDEX];
}

static esp_err_t read_frame(uint8_t *frame, uint32_t timeout_ms)
{
    size_t total_read = 0U;
    TickType_t start_tick = xTaskGetTickCount();

    while (total_read < BL0939_FRAME_SIZE_BYTES)
    {
        uint32_t remaining_timeout_ms = 0U;
        if (timeout_ms > 0U)
        {
            TickType_t elapsed_ticks = xTaskGetTickCount() - start_tick;
            uint64_t elapsed_ms = (uint64_t)elapsed_ticks * (uint64_t)portTICK_PERIOD_MS;
            if (elapsed_ms >= timeout_ms)
            {
                return ESP_ERR_TIMEOUT;
            }
            remaining_timeout_ms = timeout_ms - (uint32_t)elapsed_ms;
        }

        size_t chunk_len = 0U;
        esp_err_t ret;

        if (total_read == 0U)
        {
            uint8_t first = 0U;
            ret = uart_service_read(
                s_bl0939.uart,
                &first,
                1U,
                &chunk_len,
                remaining_timeout_ms);
            if (ret != ESP_OK)
            {
                return ret;
            }
            if (chunk_len == 0U)
            {
                return ESP_ERR_TIMEOUT;
            }

            if (first != BL0939_FRAME_HEADER_VALUE)
            {
                continue;
            }

            frame[0] = first;
            total_read = 1U;
            continue;
        }

        ret = uart_service_read(
            s_bl0939.uart,
            &frame[total_read],
            BL0939_FRAME_SIZE_BYTES - total_read,
            &chunk_len,
            remaining_timeout_ms);
        if (ret != ESP_OK)
        {
            return ret;
        }

        if (chunk_len == 0U)
        {
            return ESP_ERR_TIMEOUT;
        }

        total_read += chunk_len;
    }

    return ESP_OK;
}

esp_err_t bl0939_init(const bl0939_config_t *config)
{
    if (s_bl0939.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (config == NULL || config->uart == NULL || !calibration_is_valid(&config->calibration) ||
        (int)config->current_channel < (int)BL0939_CURRENT_CHANNEL_IA ||
        (int)config->current_channel > (int)BL0939_CURRENT_CHANNEL_SUM)
    {
        return ESP_ERR_INVALID_ARG;
    }

    s_bl0939.uart = config->uart;
    s_bl0939.calibration = config->calibration;
    s_bl0939.phase_compensation = config->phase_compensation;
    s_bl0939.current_channel = config->current_channel;
    s_bl0939.default_timeout_ms = config->default_timeout_ms;
    s_bl0939.auto_request_before_read = config->auto_request_before_read;
    s_bl0939.initialized = false;

    esp_err_t ret = write_register(
        BL0939_REG_A_CORNER,
        config->phase_compensation.corner_a,
        config->default_timeout_ms);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = write_register(
        BL0939_REG_B_CORNER,
        config->phase_compensation.corner_b,
        config->default_timeout_ms);
    if (ret != ESP_OK)
    {
        return ret;
    }

    s_bl0939.initialized = true;
    return ESP_OK;
}

esp_err_t bl0939_deinit(void)
{
    if (!s_bl0939.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    memset(&s_bl0939, 0, sizeof(s_bl0939));
    s_bl0939.initialized = false;
    return ESP_OK;
}

bool bl0939_is_initialized(void)
{
    return s_bl0939.initialized;
}

esp_err_t bl0939_request_packet(void)
{
    if (!s_bl0939.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    const uint8_t cmd[2] = {
        BL0939_CMD_READ_REQUEST,
        BL0939_CMD_READ_FULL_PACKET,
    };

    return uart_send_with_timeout(cmd, sizeof(cmd), s_bl0939.default_timeout_ms);
}

esp_err_t bl0939_read_raw(bl0939_raw_data_t *out_raw, uint32_t timeout_ms)
{
    if (!s_bl0939.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (out_raw == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t resolved_timeout_ms = resolve_timeout(timeout_ms);

    if (s_bl0939.auto_request_before_read)
    {
        esp_err_t ret = bl0939_request_packet();
        if (ret != ESP_OK)
        {
            return ret;
        }
    }

    uint8_t frame[BL0939_FRAME_SIZE_BYTES];
    esp_err_t ret = read_frame(frame, resolved_timeout_ms);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (!frame_is_valid(frame))
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    bl0939_raw_data_t raw;
    raw.ia_rms = decode_u24_le(&frame[FRAME_OFFSET_IA_RMS]);
    raw.ib_rms = decode_u24_le(&frame[FRAME_OFFSET_IB_RMS]);
    raw.v_rms = decode_u24_le(&frame[FRAME_OFFSET_V_RMS]);
    raw.cfa_cnt = decode_s24_le(&frame[FRAME_OFFSET_CFA_CNT]);
    raw.cfb_cnt = decode_s24_le(&frame[FRAME_OFFSET_CFB_CNT]);

    *out_raw = raw;
    return ESP_OK;
}

esp_err_t bl0939_read_measurements(bl0939_measurements_t *out_measurements, uint32_t timeout_ms)
{
    if (!s_bl0939.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (out_measurements == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t resolved_timeout_ms = resolve_timeout(timeout_ms);

    bl0939_raw_data_t raw;
    esp_err_t ret = bl0939_read_raw(&raw, resolved_timeout_ms);
    if (ret != ESP_OK)
    {
        return ret;
    }

    const bl0939_calibration_t *cal = &s_bl0939.calibration;
    bl0939_measurements_t measurements;
    measurements.voltage_v = (float)raw.v_rms / cal->voltage_ref;
    measurements.current_ia_a = (float)raw.ia_rms / cal->current_ref;
    measurements.current_ib_a = (float)raw.ib_rms / cal->current_ref;
    measurements.energy_a_kwh = (float)raw.cfa_cnt / cal->energy_ref;
    measurements.energy_b_kwh = (float)raw.cfb_cnt / cal->energy_ref;
    measurements.energy_kwh = measurements.energy_a_kwh + measurements.energy_b_kwh;

    switch (s_bl0939.current_channel)
    {
    case BL0939_CURRENT_CHANNEL_IA:
        measurements.current_a = measurements.current_ia_a;
        break;
    case BL0939_CURRENT_CHANNEL_IB:
        measurements.current_a = measurements.current_ib_a;
        break;
    case BL0939_CURRENT_CHANNEL_SUM:
    default:
        measurements.current_a = measurements.current_ia_a + measurements.current_ib_a;
        break;
    }

    *out_measurements = measurements;
    return ESP_OK;
}

esp_err_t bl0939_set_calibration(const bl0939_calibration_t *calibration)
{
    if (!s_bl0939.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (!calibration_is_valid(calibration))
    {
        return ESP_ERR_INVALID_ARG;
    }

    s_bl0939.calibration = *calibration;
    return ESP_OK;
}

esp_err_t bl0939_get_calibration(bl0939_calibration_t *out_calibration)
{
    if (!s_bl0939.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (out_calibration == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *out_calibration = s_bl0939.calibration;
    return ESP_OK;
}

esp_err_t bl0939_set_current_channel(bl0939_current_channel_t channel)
{
    if (!s_bl0939.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if ((int)channel < (int)BL0939_CURRENT_CHANNEL_IA || (int)channel > (int)BL0939_CURRENT_CHANNEL_SUM)
    {
        return ESP_ERR_INVALID_ARG;
    }

    s_bl0939.current_channel = channel;
    return ESP_OK;
}

esp_err_t bl0939_set_phase_compensation(const bl0939_phase_compensation_t *compensation)
{
    if (!s_bl0939.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (compensation == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = write_register(BL0939_REG_A_CORNER, compensation->corner_a, s_bl0939.default_timeout_ms);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = write_register(BL0939_REG_B_CORNER, compensation->corner_b, s_bl0939.default_timeout_ms);
    if (ret != ESP_OK)
    {
        return ret;
    }

    s_bl0939.phase_compensation = *compensation;
    return ESP_OK;
}

esp_err_t bl0939_get_phase_compensation(bl0939_phase_compensation_t *out_compensation)
{
    if (!s_bl0939.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (out_compensation == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *out_compensation = s_bl0939.phase_compensation;
    return ESP_OK;
}
