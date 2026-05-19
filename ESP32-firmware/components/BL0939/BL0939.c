#include "BL0939.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BL0939_ENABLE_UART_DIAGNOSTICS 1
#define BL0939_ENABLE_INVERSION_RECOVERY 1

#define BL0939_CMD_READ_REQUEST_BASE 0x50U
#define BL0939_CMD_READ_FULL_PACKET 0xAAU

#define BL0939_CMD_WRITE_BASE 0xA0U
#define BL0939_WRITE_PACKET_SIZE_BYTES 6U
/* Number of bytes that contribute to the write-packet checksum (all bytes except the checksum itself). */
#define BL0939_WRITE_PACKET_CHECKSUM_COVER (BL0939_WRITE_PACKET_SIZE_BYTES - 1U)

#define BL0939_UART_STOP_BITS UART_STOP_BITS_2
#define BL0939_UART_INVERSE_MASK UART_SIGNAL_INV_DISABLE

#define FRAME_OFFSET_IA_RMS 4U
#define FRAME_OFFSET_IB_RMS 7U
#define FRAME_OFFSET_V_RMS 10U
#define FRAME_OFFSET_CFA_CNT 22U
#define FRAME_OFFSET_CFB_CNT 25U
#define FRAME_CHECKSUM_INDEX (BL0939_FRAME_SIZE_BYTES - 1U)

static const char *TAG = "BL0939";

typedef struct
{
    uint8_t reg;
    uint8_t data0;
    uint8_t data1;
    uint8_t data2;
} bl0939_init_cmd_t;

typedef struct
{
    uart_service_handle_t uart;
    uint8_t device_address;
    bl0939_calibration_t calibration;
    bl0939_phase_compensation_t phase_compensation;
    bl0939_current_channel_t current_channel;
    uint8_t read_command;
    uint32_t line_inverse_mask;
    uint32_t default_timeout_ms;
    bool auto_request_before_read;
    bool initialized;
} bl0939_state_t;

static bl0939_state_t s_bl0939 = {.initialized = false};

/* Register map (from BL0939 datasheet):
 * 0x19 - USR_WRPROT:  unlock write protection (0x5A5A5A)
 * 0x1A - Mode:        operating mode / gain configuration
 * 0x18 - TPS_CTRL:    TPS / zero-crossing control
 * 0x1B - SOFT_RESET:  soft reset (write 0x47FF00 to reset, then re-init)
 * 0x10 - A_PHASE:     phase compensation for channel A
 * 0x1E - B_PHASE:     phase compensation for channel B
 */
static const bl0939_init_cmd_t s_init_cmds[] = {
    {0x19U, 0x5AU, 0x5AU, 0x5AU}, /* USR_WRPROT: unlock */
    {0x1AU, 0x55U, 0x00U, 0x00U}, /* Mode: default gain */
    {0x18U, 0x00U, 0x10U, 0x00U}, /* TPS_CTRL */
    {0x1BU, 0xFFU, 0x47U, 0x00U}, /* SOFT_RESET */
    {0x10U, 0x1CU, 0x18U, 0x00U}, /* A_PHASE */
    {0x1EU, 0x1CU, 0x18U, 0x00U}, /* B_PHASE */
};

static bool calibration_is_valid(const bl0939_calibration_t *c)
{
    return c != NULL && c->voltage_ref > 0.0f && c->current_ref > 0.0f && c->energy_ref > 0.0f;
}

static bool device_address_is_valid(uint8_t device_address)
{
    return device_address <= BL0939_DEVICE_ADDRESS_MAX;
}

static uint8_t read_command_from_address(uint8_t device_address)
{
    return (uint8_t)(BL0939_CMD_READ_REQUEST_BASE | (device_address & BL0939_DEVICE_ADDRESS_MAX));
}

static void log_tx_bytes(const char *label, const uint8_t *data, size_t len)
{
#if BL0939_ENABLE_UART_DIAGNOSTICS
    ESP_LOGI(TAG, "TX %s (%u bytes)", label, (unsigned)len);
    ESP_LOG_BUFFER_HEXDUMP(TAG, data, len, ESP_LOG_INFO);
#else
    (void)label;
    (void)data;
    (void)len;
#endif
}

static void log_rx_frame(const uint8_t *frame)
{
#if BL0939_ENABLE_UART_DIAGNOSTICS
    ESP_LOGI(TAG, "RX frame (%u bytes)", (unsigned)BL0939_FRAME_SIZE_BYTES);
    ESP_LOG_BUFFER_HEXDUMP(TAG, frame, BL0939_FRAME_SIZE_BYTES, ESP_LOG_INFO);
#else
    (void)frame;
#endif
}

static uint32_t resolve_timeout(uint32_t timeout_ms)
{
    return (timeout_ms == BL0939_TIMEOUT_USE_DEFAULT) ? s_bl0939.default_timeout_ms : timeout_ms;
}

static uint8_t write_command_from_read(void)
{
    return (uint8_t)(BL0939_CMD_WRITE_BASE | (s_bl0939.read_command & 0x0FU));
}

static bool frame_header_is_valid(uint8_t header)
{
    return header == BL0939_FRAME_HEADER_VALUE;
}

static uint32_t decode_u24_le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
}

/* FIX (decode_s24_le): The previous implementation sign-extended by OR-ing into a uint32_t
 * and then casting to int32_t, which is implementation-defined behavior in C for values that
 * do not fit the signed type's range.  We now use a union to reinterpret the bit pattern,
 * which is well-defined in C99 and later. */
static int32_t decode_s24_le(const uint8_t *p)
{
    uint32_t raw = decode_u24_le(p);
    if ((raw & 0x800000U) != 0U)
    {
        raw |= 0xFF000000U;
    }

    union
    {
        uint32_t u;
        int32_t s;
    } conv;
    conv.u = raw;
    return conv.s;
}

static esp_err_t uart_send_with_timeout(const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    (void)timeout_ms;
    return uart_service_send(s_bl0939.uart, data, len);
}

static esp_err_t set_uart_line_inverse_mask(uint32_t inverse_mask)
{
    esp_err_t ret = uart_service_set_line_inverse(s_bl0939.uart, inverse_mask);
    if (ret == ESP_OK)
    {
        s_bl0939.line_inverse_mask = inverse_mask;
    }
    return ret;
}

static esp_err_t write_register_bytes(uint8_t reg, uint8_t data0, uint8_t data1, uint8_t data2, uint32_t timeout_ms)
{
    uint8_t packet[BL0939_WRITE_PACKET_SIZE_BYTES];
    packet[0] = write_command_from_read();
    packet[1] = reg;
    packet[2] = data0;
    packet[3] = data1;
    packet[4] = data2;

    /* FIX (write_register_bytes checksum): Use the named constant
     * BL0939_WRITE_PACKET_CHECKSUM_COVER instead of a raw expression so that
     * the covered range stays correct if the packet size constant ever changes. */
    uint8_t checksum = 0U;
    for (size_t i = 0; i < BL0939_WRITE_PACKET_CHECKSUM_COVER; i++)
    {
        checksum = (uint8_t)(checksum + packet[i]);
    }
    packet[BL0939_WRITE_PACKET_CHECKSUM_COVER] = (uint8_t)(0xFFU ^ checksum);

    log_tx_bytes("write packet", packet, sizeof(packet));

    return uart_send_with_timeout(packet, sizeof(packet), timeout_ms);
}

static esp_err_t send_read_request(uint8_t read_command)
{
    const uint8_t cmd[2] = {
        read_command,
        BL0939_CMD_READ_FULL_PACKET,
    };

    log_tx_bytes("read request", cmd, sizeof(cmd));
    return uart_send_with_timeout(cmd, sizeof(cmd), s_bl0939.default_timeout_ms);
}

static esp_err_t write_register_u16(uint8_t reg, uint16_t value, uint32_t timeout_ms)
{
    return write_register_bytes(
        reg,
        (uint8_t)(value & 0xFFU),
        (uint8_t)((value >> 8) & 0xFFU),
        0x00U,
        timeout_ms);
}

static esp_err_t configure_uart_for_bl0939(void)
{
    esp_err_t ret = uart_service_set_stop_bits(s_bl0939.uart, BL0939_UART_STOP_BITS);
    if (ret != ESP_OK)
    {
        return ret;
    }

    return set_uart_line_inverse_mask(s_bl0939.line_inverse_mask);
}

static esp_err_t run_init_sequence(uint32_t timeout_ms)
{
    for (size_t i = 0; i < (sizeof(s_init_cmds) / sizeof(s_init_cmds[0])); i++)
    {
        esp_err_t ret = write_register_bytes(
            s_init_cmds[i].reg,
            s_init_cmds[i].data0,
            s_init_cmds[i].data1,
            s_init_cmds[i].data2,
            timeout_ms);
        if (ret != ESP_OK)
        {
            return ret;
        }

        vTaskDelay(pdMS_TO_TICKS(2U));
    }

    return ESP_OK;
}

static bool frame_is_valid(const uint8_t *frame)
{
    if (!frame_header_is_valid(frame[0]))
    {
#if BL0939_ENABLE_UART_DIAGNOSTICS
        ESP_LOGW(TAG, "Invalid frame header: 0x%02X", frame[0]);
#endif
        return false;
    }

    uint8_t checksum = s_bl0939.read_command;
    for (size_t i = 0; i < FRAME_CHECKSUM_INDEX; i++)
    {
        checksum = (uint8_t)(checksum + frame[i]);
    }
    checksum = (uint8_t)(0xFFU ^ checksum);

#if BL0939_ENABLE_UART_DIAGNOSTICS
    if (checksum != frame[FRAME_CHECKSUM_INDEX])
    {
        ESP_LOGW(
            TAG,
            "Checksum mismatch: expected=0x%02X actual=0x%02X",
            checksum,
            frame[FRAME_CHECKSUM_INDEX]);
    }
#endif

    return checksum == frame[FRAME_CHECKSUM_INDEX];
}

/* FIX (read_frame timeout arithmetic): The previous code cast elapsed_ms (uint64_t) to
 * uint32_t before the subtraction.  On a long-running system where elapsed_ms exceeds
 * UINT32_MAX (~49 days at 1 ms/tick) the truncated value could wrap to a value smaller
 * than timeout_ms, producing a wildly incorrect remaining_timeout.  We now saturate the
 * subtraction: if elapsed_ms >= timeout_ms we have already returned ESP_ERR_TIMEOUT, so
 * the remaining value is guaranteed to fit in uint32_t at the point of use. */
static esp_err_t read_frame(uint8_t *frame, uint32_t timeout_ms)
{
    size_t total_read = 0U;
    size_t dropped_until_header = 0U;
    TickType_t start_tick = xTaskGetTickCount();

    while (total_read < BL0939_FRAME_SIZE_BYTES)
    {
        uint32_t remaining_timeout_ms = 0U;
        if (timeout_ms > 0U)
        {
            TickType_t elapsed_ticks = xTaskGetTickCount() - start_tick;
            uint64_t elapsed_ms = (uint64_t)elapsed_ticks * (uint64_t)portTICK_PERIOD_MS;
            if (elapsed_ms >= (uint64_t)timeout_ms)
            {
#if BL0939_ENABLE_UART_DIAGNOSTICS
                ESP_LOGW(TAG, "Frame read timeout after %u/%u byte(s)",
                         (unsigned)total_read,
                         (unsigned)BL0939_FRAME_SIZE_BYTES);
#endif
                return ESP_ERR_TIMEOUT;
            }
            /* elapsed_ms < timeout_ms here, so the difference fits in uint32_t. */
            remaining_timeout_ms = (uint32_t)((uint64_t)timeout_ms - elapsed_ms);
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
#if BL0939_ENABLE_UART_DIAGNOSTICS
                ESP_LOGW(TAG,
                         "Frame read timeout waiting for valid header (dropped=%u)",
                         (unsigned)dropped_until_header);
#endif
                return ESP_ERR_TIMEOUT;
            }

            if (!frame_header_is_valid(first))
            {
                dropped_until_header++;
                continue;
            }

#if BL0939_ENABLE_UART_DIAGNOSTICS
            if (dropped_until_header > 0U)
            {
                ESP_LOGW(TAG, "Dropped %u byte(s) before frame header sync", (unsigned)dropped_until_header);
            }
#endif

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
#if BL0939_ENABLE_UART_DIAGNOSTICS
            ESP_LOGW(TAG,
                     "Frame read timeout after header (%u/%u byte(s))",
                     (unsigned)total_read,
                     (unsigned)BL0939_FRAME_SIZE_BYTES);
#endif
            return ESP_ERR_TIMEOUT;
        }

        total_read += chunk_len;
    }

    log_rx_frame(frame);

    return ESP_OK;
}

static bool should_try_inversion_recovery(esp_err_t err)
{
    return err == ESP_ERR_TIMEOUT || err == ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t attempt_inversion_recovery(uint8_t *frame, uint32_t timeout_ms)
{
#if BL0939_ENABLE_INVERSION_RECOVERY
    const uint8_t original_device_address = s_bl0939.device_address;
    const uint32_t original_mask = s_bl0939.line_inverse_mask;

    /* Candidate masks are tried in order.  Duplicates are skipped below.
     * original_mask is intentionally placed first so it is always probed
     * with every candidate address (the skip condition only fires when BOTH
     * mask AND address match the original combination). */
    const uint32_t inverse_candidates[] = {
        original_mask,
        (uint32_t)UART_SIGNAL_INV_DISABLE,
        (uint32_t)UART_SIGNAL_RXD_INV,
        (uint32_t)UART_SIGNAL_TXD_INV,
        (uint32_t)(UART_SIGNAL_RXD_INV | UART_SIGNAL_TXD_INV),
    };

    esp_err_t last_err = ESP_ERR_TIMEOUT;

    ESP_LOGW(TAG,
             "Attempting UART/protocol recovery (addr=%u cmd=0x%02X mask=0x%02X)",
             (unsigned)original_device_address,
             (unsigned)s_bl0939.read_command,
             (unsigned)original_mask);

    for (size_t i = 0; i < (sizeof(inverse_candidates) / sizeof(inverse_candidates[0])); i++)
    {
        const uint32_t candidate_mask = inverse_candidates[i];

        /* Skip duplicate masks — no need to repeat the same electrical configuration. */
        bool duplicate_mask = false;
        for (size_t prior = 0; prior < i; prior++)
        {
            if (inverse_candidates[prior] == candidate_mask)
            {
                duplicate_mask = true;
                break;
            }
        }
        if (duplicate_mask)
        {
            continue;
        }

        esp_err_t ret = set_uart_line_inverse_mask(candidate_mask);
        if (ret != ESP_OK)
        {
            last_err = ret;
            continue;
        }

        /* FIX (attempt_inversion_recovery): Previously, inverse_candidates[0] equals
         * original_mask, so the inner skip condition
         *   (candidate_mask == original_mask && candidate_address == original_device_address)
         * fired for every address in the first outer iteration, meaning the original mask
         * was never probed with any address other than the current one.
         *
         * The fix separates the two concerns:
         *   - cmd_step == 0  → original address (always tried, regardless of mask)
         *   - cmd_step  > 0  → remaining addresses 0..BL0939_DEVICE_ADDRESS_MAX
         *
         * The (mask, address) pair that matches the currently-configured combination is
         * skipped because it was already attempted by the caller before recovery was
         * triggered; all other combinations are probed. */
        for (uint16_t cmd_step = 0U; cmd_step <= (uint16_t)BL0939_DEVICE_ADDRESS_MAX + 1U; cmd_step++)
        {
            uint8_t candidate_address;
            if (cmd_step == 0U)
            {
                candidate_address = original_device_address;
            }
            else
            {
                candidate_address = (uint8_t)(cmd_step - 1U);
            }

            /* Skip the exact (mask, address) pair we started with — it already failed. */
            if (candidate_mask == original_mask && candidate_address == original_device_address)
            {
                continue;
            }

            s_bl0939.device_address = candidate_address;
            s_bl0939.read_command = read_command_from_address(candidate_address);

            ret = run_init_sequence(timeout_ms);
            if (ret != ESP_OK)
            {
                last_err = ret;
                continue;
            }

            ret = write_register_u16(BL0939_REG_A_CORNER, s_bl0939.phase_compensation.corner_a, timeout_ms);
            if (ret != ESP_OK)
            {
                last_err = ret;
                continue;
            }

            ret = write_register_u16(BL0939_REG_B_CORNER, s_bl0939.phase_compensation.corner_b, timeout_ms);
            if (ret != ESP_OK)
            {
                last_err = ret;
                continue;
            }

            /* FIX (flush error logging): Previously flush errors were silently discarded.
             * Log them in diagnostic mode so they appear in field logs. The flush failure
             * is non-fatal for the recovery attempt; we still try to read. */
            ret = uart_service_flush_input(s_bl0939.uart);
#if BL0939_ENABLE_UART_DIAGNOSTICS
            if (ret != ESP_OK)
            {
                ESP_LOGW(TAG, "Flush failed during recovery (addr=%u mask=0x%02X): %s",
                         (unsigned)candidate_address,
                         (unsigned)candidate_mask,
                         esp_err_to_name(ret));
            }
#endif

            ret = send_read_request(s_bl0939.read_command);
            if (ret != ESP_OK)
            {
                last_err = ret;
                continue;
            }

            ret = read_frame(frame, timeout_ms);
            if (ret != ESP_OK)
            {
                last_err = ret;
                continue;
            }

            if (frame_is_valid(frame))
            {
                /* NOTE: s_bl0939.device_address and s_bl0939.read_command are intentionally
                 * left updated here — recovery has found a working configuration and the
                 * driver must continue using it.  Callers should be aware that these fields
                 * may differ from the values passed to bl0939_init() after a successful
                 * recovery. */
                ESP_LOGI(TAG,
                         "UART/protocol recovered with addr=%u cmd=0x%02X mask=0x%02X",
                         (unsigned)s_bl0939.device_address,
                         (unsigned)s_bl0939.read_command,
                         (unsigned)candidate_mask);
                return ESP_OK;
            }

            last_err = ESP_ERR_INVALID_RESPONSE;
        }
    }

    /* Restore original configuration before returning failure. */
    s_bl0939.device_address = original_device_address;
    s_bl0939.read_command = read_command_from_address(original_device_address);
    (void)set_uart_line_inverse_mask(original_mask);
    return last_err;
#else
    (void)frame;
    (void)timeout_ms;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t bl0939_init(const bl0939_config_t *config)
{
    if (s_bl0939.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (config == NULL || config->uart == NULL || !calibration_is_valid(&config->calibration) ||
        !device_address_is_valid(config->device_address) ||
        (int)config->current_channel < (int)BL0939_CURRENT_CHANNEL_IA ||
        (int)config->current_channel > (int)BL0939_CURRENT_CHANNEL_SUM)
    {
        return ESP_ERR_INVALID_ARG;
    }

    s_bl0939.uart = config->uart;
    s_bl0939.device_address = config->device_address;
    s_bl0939.calibration = config->calibration;
    s_bl0939.phase_compensation = config->phase_compensation;
    s_bl0939.current_channel = config->current_channel;
    s_bl0939.read_command = read_command_from_address(config->device_address);
    s_bl0939.line_inverse_mask = (uint32_t)BL0939_UART_INVERSE_MASK;
    s_bl0939.default_timeout_ms = config->default_timeout_ms;
    s_bl0939.auto_request_before_read = config->auto_request_before_read;

#if BL0939_ENABLE_UART_DIAGNOSTICS
    ESP_LOGI(TAG,
             "Init with addr=%u cmd=0x%02X stop=1.5",
             (unsigned)s_bl0939.device_address,
             (unsigned)s_bl0939.read_command);
#endif

    esp_err_t ret = configure_uart_for_bl0939();
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = run_init_sequence(config->default_timeout_ms);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = write_register_u16(
        BL0939_REG_A_CORNER,
        config->phase_compensation.corner_a,
        config->default_timeout_ms);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = write_register_u16(
        BL0939_REG_B_CORNER,
        config->phase_compensation.corner_b,
        config->default_timeout_ms);
    if (ret != ESP_OK)
    {
        return ret;
    }

    /* FIX (bl0939_init flush error logging): flush errors were silently discarded.
     * Log them so they are visible in diagnostics; the flush is best-effort at init. */
    ret = uart_service_flush_input(s_bl0939.uart);
#if BL0939_ENABLE_UART_DIAGNOSTICS
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "Input flush failed at init: %s", esp_err_to_name(ret));
    }
#endif

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
    /* initialized is already zero from memset; this assignment is redundant but
     * kept for clarity and to satisfy static analysers. */
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

    return send_read_request(s_bl0939.read_command);
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
        esp_err_t ret = uart_service_flush_input(s_bl0939.uart);
#if BL0939_ENABLE_UART_DIAGNOSTICS
        if (ret != ESP_OK)
        {
            ESP_LOGW(TAG, "Input flush failed before read: %s", esp_err_to_name(ret));
        }
#endif
        if (ret != ESP_OK)
        {
            return ret;
        }

        ret = bl0939_request_packet();
        if (ret != ESP_OK)
        {
            return ret;
        }
    }

    uint8_t frame[BL0939_FRAME_SIZE_BYTES];
    esp_err_t ret = read_frame(frame, resolved_timeout_ms);
    esp_err_t frame_status = ret;

    if (ret == ESP_OK && !frame_is_valid(frame))
    {
        frame_status = ESP_ERR_INVALID_RESPONSE;
    }

    if (frame_status != ESP_OK && s_bl0939.auto_request_before_read && should_try_inversion_recovery(frame_status))
    {
        esp_err_t recovery_ret = attempt_inversion_recovery(frame, resolved_timeout_ms);
        if (recovery_ret == ESP_OK)
        {
            frame_status = ESP_OK;
        }
    }

    if (frame_status != ESP_OK)
    {
        return frame_status;
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

    esp_err_t ret = write_register_u16(BL0939_REG_A_CORNER, compensation->corner_a, s_bl0939.default_timeout_ms);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = write_register_u16(BL0939_REG_B_CORNER, compensation->corner_b, s_bl0939.default_timeout_ms);
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
