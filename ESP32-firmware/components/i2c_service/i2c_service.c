/**
 * @file i2c_service.c
 * @brief I2C service implementation - see i2c_service.h for API documentation.
 */

#include "i2c_service.h"

#include <stdbool.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define I2C_MUTEX_TIMEOUT_MS 100
#define I2C_XFER_TIMEOUT_MS 100
#define I2C_SERVICE_MAX_DEVICES 8

typedef struct
{
    uint8_t dev_addr;
    i2c_master_dev_handle_t dev_handle;
    bool in_use;
} i2c_service_device_entry_t;

typedef struct
{
    bool initialized;
    uint32_t clk_hz;
    i2c_master_bus_handle_t bus_handle;
    SemaphoreHandle_t mutex;
    i2c_service_device_entry_t devices[I2C_SERVICE_MAX_DEVICES];
} i2c_service_state_t;

static i2c_service_state_t s_state = {0};
static const char *TAG = "i2c_service";

static TickType_t i2c_service_mutex_timeout_ticks(void)
{
    TickType_t ticks = pdMS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS);
    return (ticks == 0) ? 1 : ticks;
}

static bool i2c_service_is_dev_addr_valid(uint8_t dev_addr)
{
    return dev_addr <= 0x7FU;
}

static i2c_service_device_entry_t *i2c_service_find_device(uint8_t dev_addr)
{
    for (int i = 0; i < I2C_SERVICE_MAX_DEVICES; i++)
    {
        if (s_state.devices[i].in_use && s_state.devices[i].dev_addr == dev_addr)
        {
            return &s_state.devices[i];
        }
    }
    return NULL;
}

static i2c_service_device_entry_t *i2c_service_alloc_device(void)
{
    for (int i = 0; i < I2C_SERVICE_MAX_DEVICES; i++)
    {
        if (!s_state.devices[i].in_use)
        {
            return &s_state.devices[i];
        }
    }
    return NULL;
}

static esp_err_t i2c_service_get_or_create_device(uint8_t dev_addr, i2c_master_dev_handle_t *out_handle)
{
    if (out_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_service_device_entry_t *entry = i2c_service_find_device(dev_addr);
    if (entry != NULL)
    {
        *out_handle = entry->dev_handle;
        return ESP_OK;
    }

    entry = i2c_service_alloc_device();
    if (entry == NULL)
    {
        ESP_LOGW(TAG, "Device table full, falling back to temporary handle");
        return i2c_service_add_temp_device(dev_addr, out_handle);
    }

    esp_err_t ret = i2c_service_add_temp_device(dev_addr, &entry->dev_handle);
    if (ret != ESP_OK)
    {
        return ret;
    }

    entry->dev_addr = dev_addr;
    entry->in_use = true;
    *out_handle = entry->dev_handle;
    return ESP_OK;
}

static esp_err_t i2c_service_lock(void)
{
    if (xSemaphoreTake(s_state.mutex, i2c_service_mutex_timeout_ticks()) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

static void i2c_service_unlock(void)
{
    (void)xSemaphoreGive(s_state.mutex);
}

static esp_err_t i2c_service_add_temp_device(uint8_t dev_addr, i2c_master_dev_handle_t *out_dev)
{
    if (out_dev == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dev_addr,
        .scl_speed_hz = s_state.clk_hz,
    };

    return i2c_master_bus_add_device(s_state.bus_handle, &dev_cfg, out_dev);
}

esp_err_t i2c_service_init(i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t clk_hz)
{
    if (s_state.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (clk_hz == 0 || !GPIO_IS_VALID_GPIO(sda) || !GPIO_IS_VALID_GPIO(scl) ||
        ((int)port < 0) || (port >= I2C_NUM_MAX))
    {
        return ESP_ERR_INVALID_ARG;
    }

    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
    if (mutex == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = (i2c_port_num_t)port,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags.enable_internal_pullup = 1,
        .flags.allow_pd = 0,
    };

    i2c_master_bus_handle_t bus = NULL;
    esp_err_t ret = i2c_new_master_bus(&bus_cfg, &bus);
    if (ret != ESP_OK)
    {
        vSemaphoreDelete(mutex);
        return ret;
    }

    s_state.bus_handle = bus;
    s_state.mutex = mutex;
    s_state.clk_hz = clk_hz;
    s_state.initialized = true;

    return ESP_OK;
}

esp_err_t i2c_service_deinit(void)
{
    if (!s_state.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = i2c_service_lock();
    if (ret != ESP_OK)
    {
        return ret;
    }

    for (int i = 0; i < I2C_SERVICE_MAX_DEVICES; i++)
    {
        if (s_state.devices[i].in_use)
        {
            (void)i2c_master_bus_rm_device(s_state.devices[i].dev_handle);
            s_state.devices[i].in_use = false;
        }
    }

    ret = i2c_del_master_bus(s_state.bus_handle);

    i2c_service_unlock();

    if (ret != ESP_OK)
    {
        return ret;
    }

    vSemaphoreDelete(s_state.mutex);
    memset(&s_state, 0, sizeof(s_state));

    return ESP_OK;
}

esp_err_t i2c_service_write(uint8_t dev_addr, const uint8_t *data, size_t len)
{
    if (!s_state.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (data == NULL || len == 0 || !i2c_service_is_dev_addr_valid(dev_addr))
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = i2c_service_lock();
    if (ret != ESP_OK)
    {
        return ret;
    }

    i2c_master_dev_handle_t dev_handle = NULL;
    ret = i2c_service_get_or_create_device(dev_addr, &dev_handle);
    if (ret == ESP_OK)
    {
        ret = i2c_master_transmit(dev_handle, data, len, I2C_XFER_TIMEOUT_MS);
    }

    i2c_service_unlock();
    return ret;
}

esp_err_t i2c_service_write_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *out, size_t len)
{
    if (!s_state.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (out == NULL || len == 0 || !i2c_service_is_dev_addr_valid(dev_addr))
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = i2c_service_lock();
    if (ret != ESP_OK)
    {
        return ret;
    }

    i2c_master_dev_handle_t dev_handle = NULL;
    ret = i2c_service_get_or_create_device(dev_addr, &dev_handle);
    if (ret == ESP_OK)
    {
        ret = i2c_master_transmit_receive(dev_handle,
                                          &reg_addr,
                                          sizeof(reg_addr),
                                          out,
                                          len,
                                          I2C_XFER_TIMEOUT_MS);
    }

    i2c_service_unlock();
    return ret;
}

i2c_master_bus_handle_t i2c_service_get_bus_handle(void)
{
    if (!s_state.initialized)
    {
        return NULL;
    }

    return s_state.bus_handle;
}

esp_err_t i2c_service_remove_device(uint8_t dev_addr)
{
    if (!s_state.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = i2c_service_lock();
    if (ret != ESP_OK)
    {
        return ret;
    }

    i2c_service_device_entry_t *entry = i2c_service_find_device(dev_addr);
    if (entry == NULL)
    {
        i2c_service_unlock();
        return ESP_ERR_NOT_FOUND;
    }

    ret = i2c_master_bus_rm_device(entry->dev_handle);
    entry->in_use = false;
    entry->dev_handle = NULL;

    i2c_service_unlock();
    return ret;
}