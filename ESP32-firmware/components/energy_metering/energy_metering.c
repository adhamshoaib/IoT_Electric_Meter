#include "energy_metering.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

typedef struct {
    energy_metering_calibration_t cal;
    float total_energy_kwh;
    float energy_divisor;
    int32_t prev_cfa_cnt;
    int32_t prev_cfb_cnt;
    bool counter_valid;
    bool initialized;
    bool sample_valid;
    bool task_stop_requested;
    energy_metering_data_t latest_data;
    TaskHandle_t task_handle;
    uint32_t task_period_ms;
    SemaphoreHandle_t lock;
} energy_metering_t;

static energy_metering_t s_ctx;
static const energy_metering_task_config_t k_default_task_cfg = ENERGY_METERING_TASK_CONFIG_DEFAULT();

static bool lock_take(void)
{
    if (s_ctx.lock == NULL)
    {
        return false;
    }

    return xSemaphoreTake(s_ctx.lock, portMAX_DELAY) == pdTRUE;
}

static void lock_give(void)
{
    if (s_ctx.lock != NULL)
    {
        (void)xSemaphoreGive(s_ctx.lock);
    }
}

static void energy_metering_task_entry(void *arg);

static bool calibration_is_valid(const energy_metering_calibration_t *cal)
{
    return (cal != NULL) &&
           (cal->vrms_scale > 0.0f) &&
           (cal->vref > 0.0f) &&
           (cal->divider_ratio > 0.0f) &&
           (cal->vac_fine_gain > 0.0f) &&
           (cal->irms_scale > 0.0f) &&
           (cal->ia_pin_fine_gain > 0.0f) &&
           (cal->ia_cal_a_per_mv > 0.0f) &&
           (cal->energy_ref > 0.0f) &&
           (cal->cf_count_scale > 0.0f) &&
           (cal->vac_noise_floor_v >= 0.0f) &&
           (cal->ia_noise_floor_a >= 0.0f) &&
           (cal->vp_noise_floor_mv >= 0.0f);
}

static uint32_t vrms_raw_apply_zero_offset(uint32_t v_rms_raw)
{
    if (v_rms_raw <= s_ctx.cal.vrms_zero_offset)
    {
        return 0U;
    }

    return v_rms_raw - s_ctx.cal.vrms_zero_offset;
}

static float vrms_raw_to_mains_v(uint32_t v_rms_raw)
{
    const uint32_t corrected_raw = vrms_raw_apply_zero_offset(v_rms_raw);
    const float vp_mv = ((float)corrected_raw * s_ctx.cal.vref) / s_ctx.cal.vrms_scale;
    const float mains_v = ((vp_mv * s_ctx.cal.divider_ratio) / 1000.0f) * s_ctx.cal.vac_fine_gain;

    return (mains_v < s_ctx.cal.vac_noise_floor_v) ? 0.0f : mains_v;
}

static float irms_raw_to_ip_mv(uint32_t i_rms_raw)
{
    const float ip_mv = (((float)i_rms_raw * s_ctx.cal.vref) / s_ctx.cal.irms_scale) *
                        s_ctx.cal.ia_pin_fine_gain;
    return (ip_mv < s_ctx.cal.vp_noise_floor_mv) ? 0.0f : ip_mv;
}

static float irms_raw_to_amps(uint32_t i_rms_raw)
{
    const float ip_mv = irms_raw_to_ip_mv(i_rms_raw);
    const float amps = ip_mv * s_ctx.cal.ia_cal_a_per_mv;

    return (amps < s_ctx.cal.ia_noise_floor_a) ? 0.0f : amps;
}

static bool counter_wrapped(int32_t curr, int32_t prev)
{
    const int32_t delta = curr - prev;
    return ((prev >= 0) && (curr < 0) && (delta < 0)) ||
           ((prev < 0) && (curr >= 0) && (delta > 0));
}

static float frame_energy_kwh(const bl0939_raw_data_t *raw)
{
    if (raw == NULL)
    {
        return 0.0f;
    }

    if (!s_ctx.counter_valid)
    {
        s_ctx.prev_cfa_cnt = raw->cfa_cnt;
        s_ctx.prev_cfb_cnt = raw->cfb_cnt;
        s_ctx.counter_valid = true;
        return 0.0f;
    }

    if (counter_wrapped(raw->cfa_cnt, s_ctx.prev_cfa_cnt) ||
        counter_wrapped(raw->cfb_cnt, s_ctx.prev_cfb_cnt))
    {
        s_ctx.prev_cfa_cnt = raw->cfa_cnt;
        s_ctx.prev_cfb_cnt = raw->cfb_cnt;
        return 0.0f;
    }

    const int32_t delta_cfa = raw->cfa_cnt - s_ctx.prev_cfa_cnt;
    const int32_t delta_cfb = raw->cfb_cnt - s_ctx.prev_cfb_cnt;

    s_ctx.prev_cfa_cnt = raw->cfa_cnt;
    s_ctx.prev_cfb_cnt = raw->cfb_cnt;

    const int32_t delta_total = delta_cfa + delta_cfb;
    if (delta_total <= 0)
    {
        return 0.0f;
    }

    return (float)delta_total / s_ctx.energy_divisor;
}

static esp_err_t energy_metering_read_locked(energy_metering_data_t *out, uint32_t timeout_ms)
{
    bl0939_raw_data_t raw;
    esp_err_t ret = bl0939_read_raw(&raw, timeout_ms);
    if (ret != ESP_OK)
    {
        return ret;
    }

    s_ctx.total_energy_kwh += frame_energy_kwh(&raw);

    energy_metering_data_t data;
    data.voltage_v = vrms_raw_to_mains_v(raw.v_rms);
    data.current_a = irms_raw_to_amps(raw.ia_rms);
    data.total_energy_kwh = s_ctx.total_energy_kwh;

    s_ctx.latest_data = data;
    s_ctx.sample_valid = true;
    *out = data;

    return ESP_OK;
}

static energy_metering_task_config_t task_config_resolve(const energy_metering_task_config_t *config)
{
    energy_metering_task_config_t resolved = *config;

    if (resolved.task_name == NULL)
    {
        resolved.task_name = k_default_task_cfg.task_name;
    }

    if (resolved.stack_size == 0U)
    {
        resolved.stack_size = k_default_task_cfg.stack_size;
    }

    if (resolved.priority == 0U)
    {
        resolved.priority = k_default_task_cfg.priority;
    }

    if (resolved.period_ms == 0U)
    {
        resolved.period_ms = k_default_task_cfg.period_ms;
    }

    return resolved;
}

esp_err_t energy_metering_init(const energy_metering_config_t *config)
{
    if (config == NULL || !calibration_is_valid(&config->calibration))
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_ctx.lock == NULL)
    {
        s_ctx.lock = xSemaphoreCreateMutex();
        if (s_ctx.lock == NULL)
        {
            return ESP_ERR_NO_MEM;
        }
    }

    if (!lock_take())
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_ctx.task_handle != NULL)
    {
        lock_give();
        return ESP_ERR_INVALID_STATE;
    }

    s_ctx.cal = config->calibration;
    s_ctx.energy_divisor = s_ctx.cal.energy_ref * s_ctx.cal.cf_count_scale;
    s_ctx.total_energy_kwh = 0.0f;
    s_ctx.prev_cfa_cnt = 0;
    s_ctx.prev_cfb_cnt = 0;
    s_ctx.counter_valid = false;
    s_ctx.sample_valid = false;
    s_ctx.task_stop_requested = false;
    s_ctx.task_period_ms = k_default_task_cfg.period_ms;
    s_ctx.initialized = true;

    lock_give();

    return ESP_OK;
}

esp_err_t energy_metering_read(energy_metering_data_t *out, uint32_t timeout_ms)
{
    if (!s_ctx.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!lock_take())
    {
        return ESP_ERR_TIMEOUT;
    }

    if ((s_ctx.task_handle != NULL) &&
        (xTaskGetCurrentTaskHandle() != s_ctx.task_handle))
    {
        lock_give();
        return ESP_ERR_INVALID_STATE;
    }

    const esp_err_t ret = energy_metering_read_locked(out, timeout_ms);
    lock_give();

    return ret;
}

esp_err_t energy_metering_get_latest(energy_metering_data_t *out)
{
    if (!s_ctx.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!lock_take())
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (!s_ctx.sample_valid)
    {
        lock_give();
        return ESP_ERR_INVALID_STATE;
    }

    *out = s_ctx.latest_data;
    lock_give();

    return ESP_OK;
}

esp_err_t energy_metering_start_task(const energy_metering_task_config_t *config)
{
    if (config == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_ctx.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    const energy_metering_task_config_t task_cfg = task_config_resolve(config);
    if ((task_cfg.stack_size == 0U) ||
        (task_cfg.period_ms == 0U) ||
        (task_cfg.priority == 0U) ||
        (task_cfg.priority >= (uint32_t)configMAX_PRIORITIES))
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!lock_take())
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_ctx.task_handle != NULL)
    {
        lock_give();
        return ESP_ERR_INVALID_STATE;
    }

    s_ctx.task_period_ms = task_cfg.period_ms;
    s_ctx.task_stop_requested = false;

    const BaseType_t task_created = xTaskCreate(energy_metering_task_entry,
                                                 task_cfg.task_name,
                                                 task_cfg.stack_size,
                                                 NULL,
                                                 (UBaseType_t)task_cfg.priority,
                                                 &s_ctx.task_handle);
    if (task_created != pdPASS)
    {
        s_ctx.task_handle = NULL;
        lock_give();
        return ESP_ERR_NO_MEM;
    }

    lock_give();
    return ESP_OK;
}

esp_err_t energy_metering_stop_task(void)
{
    if (!s_ctx.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (!lock_take())
    {
        return ESP_ERR_INVALID_STATE;
    }

    const TaskHandle_t task_handle = s_ctx.task_handle;
    if (task_handle == NULL)
    {
        lock_give();
        return ESP_ERR_INVALID_STATE;
    }

    s_ctx.task_stop_requested = true;
    lock_give();

    if (xTaskGetCurrentTaskHandle() == task_handle)
    {
        return ESP_OK;
    }

    const uint32_t timeout_ticks = pdMS_TO_TICKS(5000U);
    const TickType_t start_ticks = xTaskGetTickCount();

    while (true)
    {
        if (lock_take())
        {
            const bool task_stopped = (s_ctx.task_handle == NULL);
            lock_give();
            if (task_stopped)
            {
                break;
            }
        }

        if ((xTaskGetTickCount() - start_ticks) >= timeout_ticks)
        {
            return ESP_ERR_TIMEOUT;
        }

        vTaskDelay(pdMS_TO_TICKS(10U));
    }

    return ESP_OK;
}

void energy_metering_reset_energy(void)
{
    if (!s_ctx.initialized)
    {
        return;
    }

    if (!lock_take())
    {
        return;
    }

    s_ctx.total_energy_kwh = 0.0f;
    s_ctx.prev_cfa_cnt = 0;
    s_ctx.prev_cfb_cnt = 0;
    s_ctx.counter_valid = false;

    if (s_ctx.sample_valid)
    {
        s_ctx.latest_data.total_energy_kwh = 0.0f;
    }

    lock_give();
}

esp_err_t energy_metering_deinit(void)
{
    if (!s_ctx.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    (void)energy_metering_stop_task();

    if (s_ctx.lock != NULL)
    {
        vSemaphoreDelete(s_ctx.lock);
        s_ctx.lock = NULL;
    }

    s_ctx.initialized = false;
    s_ctx.task_handle = NULL;

    return ESP_OK;
}

static void energy_metering_task_entry(void *arg)
{
    (void)arg;

    while (true)
    {
        if (!lock_take())
        {
            vTaskDelete(NULL);
            return;
        }

        const bool stop_requested = s_ctx.task_stop_requested;
        const uint32_t period_ms = s_ctx.task_period_ms;
        lock_give();

        if (stop_requested)
        {
            break;
        }

        energy_metering_data_t sample;
        if (lock_take())
        {
            (void)energy_metering_read_locked(&sample, BL0939_TIMEOUT_USE_DEFAULT);
            lock_give();
        }

        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }

    if (lock_take())
    {
        s_ctx.task_handle = NULL;
        s_ctx.task_stop_requested = false;
        lock_give();
    }

    vTaskDelete(NULL);
}
