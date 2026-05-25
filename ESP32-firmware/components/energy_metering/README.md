# energy_metering

ESP-IDF component that wraps the BL0939 energy-monitoring IC driver into a thread-safe, high-level API. It handles raw-to-physical-unit conversion, energy accumulation, load-detection hysteresis, and optional background sampling via a dedicated FreeRTOS task.

## Dependencies

- `BL0939` — low-level driver for the BL0939 IC (UART communication, register readout)
- `freertos` — FreeRTOS mutex, tasks, and atomic operations

## Architecture

```
Application
    |
    | energy_metering_init / energy_metering_read / energy_metering_get_latest
    v
energy_metering.c    <-- this component
    |
    | bl0939_read_raw
    v
BL0939 component     <-- UART-level driver
    |
    | uart_service_read / uart_service_write
    v
UART hardware
```

There are two usage modes:

1. **Direct reads** — the application calls `energy_metering_read()` periodically (no background task).
2. **Background task** — a dedicated FreeRTOS task calls `energy_metering_read()` on a timer; other tasks use the lock-free `energy_metering_get_latest()` to retrieve the most recent sample.

## Configuration

### `energy_metering_calibration_t`

Physical constants that map BL0939 register values to real-world voltage and current. Use `ENERGY_METERING_CALIBRATION_DEFAULT()` to get the values matching the hardware prototype; adjust for your own PCB.

| Field | Description |
|---|---|
| `vrms_scale` | BL0939 Vrms full-scale count (default 79931) |
| `vref` | Internal voltage reference (default 1.218 V) |
| `divider_ratio` | External resistor-divider ratio (Vmains / Vpin) |
| `vac_fine_gain` | Empirical trim for the voltage channel |
| `vrms_zero_offset` | ADC offset counts subtracted before scaling |
| `vac_noise_floor_v` | Voltage readings below this are clamped to 0 V |
| `irms_scale` | BL0939 Irms full-scale count (default 324004) |
| `ia_pin_fine_gain` | Empirical trim for current sense amplifier |
| `ia_cal_a_per_mv` | Current in amperes per mV at the sense pin |
| `ia_noise_floor_a` | Current readings below this are clamped to 0 A |
| `vp_noise_floor_mv` | Sense-pin voltage noise floor (mV) |
| `energy_ref` | BL0939 energy reference (default 3304) |
| `cf_count_scale` | CF pulse counter scale factor (default 20000) |

### `energy_metering_config_t`

| Field | Description |
|---|---|
| `calibration` | Calibration constants (see above) |
| `load_threshold_a` | Current above this indicates load connected; 0 = feature disabled |
| `load_hysteresis_a` | Hysteresis band around the threshold to prevent chattering |

### `energy_metering_task_config_t`

| Field | Description |
|---|---|
| `task_name` | FreeRTOS task name (NULL -> `"energy_meter"`) |
| `stack_size` | Stack size in bytes (default 4096) |
| `priority` | FreeRTOS priority (default 5) |
| `period_ms` | Polling period in ms (default 1000) |

## API

### `energy_metering_init`

```c
esp_err_t energy_metering_init(const energy_metering_config_t *config);
```

Initialises the driver. Must be called after `bl0939_init()`. Resets the energy accumulator and pulse-counter state.

- **Parameters**: `config` — calibration and load-detection settings.
- **Returns**: `ESP_OK`, `ESP_ERR_INVALID_ARG`, `ESP_ERR_NO_MEM`, `ESP_ERR_INVALID_STATE`.

### `energy_metering_read`

```c
esp_err_t energy_metering_read(energy_metering_data_t *out, uint32_t timeout_ms);
```

Reads one measurement frame from the BL0939, converts to physical units, and accumulates energy.

The first call seeds the CFA/CFB counter baselines and returns 0 kWh; subsequent calls return the delta since the previous call.

If the background task is running, only that task may call this function directly.

- **Parameters**: `out` — populated on success; `timeout_ms` — passed to `bl0939_read_raw()`.
- **Returns**: `ESP_OK` or propagated error from `bl0939_read_raw()`.

### `energy_metering_get_latest`

```c
esp_err_t energy_metering_get_latest(energy_metering_data_t *out);
```

Returns the most recent sample captured by the background task without touching the BL0939 bus. Safe to call from any task.

- **Parameters**: `out` — receives the cached reading.
- **Returns**: `ESP_OK`, `ESP_ERR_INVALID_STATE` if no sample exists yet.

### `energy_metering_start_task`

```c
esp_err_t energy_metering_start_task(const energy_metering_task_config_t *config);
```

Creates a FreeRTOS task that calls `energy_metering_read()` periodically.

- **Parameters**: `config` — task parameters (must be non-NULL).
- **Returns**: `ESP_OK`, `ESP_ERR_INVALID_STATE` if already running, `ESP_ERR_NO_MEM`.

### `energy_metering_stop_task`

```c
esp_err_t energy_metering_stop_task(void);
```

Signals the background task to stop and waits up to 5 seconds for it to exit.

- **Returns**: `ESP_OK`, `ESP_ERR_TIMEOUT` if the task does not stop in time.

### `energy_metering_reset_energy`

```c
void energy_metering_reset_energy(void);
```

Resets the accumulated energy counter to zero. The CFA/CFB baselines are re-seeded on the next read. Thread-safe.

### `energy_metering_is_load_connected`

```c
bool energy_metering_is_load_connected(void);
```

Returns the current load-connected state based on the latest sample. When no background task is running, this function performs an on-demand read. Thread-safe.

### `energy_metering_deinit`

```c
esp_err_t energy_metering_deinit(void);
```

Stops the background task (if running) and deletes the internal mutex. After deinit, `energy_metering_init()` can be called again.

- **Returns**: `ESP_OK`, `ESP_ERR_INVALID_STATE` if not initialised.

## Usage example

```c
#include "energy_metering.h"

/* Must call bl0939_init() first */

const energy_metering_config_t em_cfg = ENERGY_METERING_CONFIG_DEFAULT();
ESP_ERROR_CHECK(energy_metering_init(&em_cfg));

/* Start background sampling */
const energy_metering_task_config_t task_cfg = {
    .task_name = "my_meter",
    .stack_size = 4096,
    .priority = 5,
    .period_ms = 1000,
};
ESP_ERROR_CHECK(energy_metering_start_task(&task_cfg));

/* In another task: read latest sample without touching BL0939 */
energy_metering_data_t data;
if (energy_metering_get_latest(&data) == ESP_OK) {
    printf("%.2f V  %.3f A  %.4f kWh\n",
           data.voltage_v, data.current_a, data.total_energy_kwh);
}

/* Check load state */
if (energy_metering_is_load_connected()) {
    /* load is drawing current */
}
```

## Load detection

Load detection uses a simple threshold with hysteresis:

- **Engage**: current rises above `load_threshold_a + hysteresis / 2`
- **Release**: current falls below `load_threshold_a - hysteresis / 2`

This prevents rapid toggling when the load current hovers near the threshold. The `energy_metering_data_t.load_connected` field and `energy_metering_is_load_connected()` function both reflect this state.

## Energy accumulation

Energy is derived from the BL0939's CFA and CFB pulse counters:

```
delta_energy_kWh = (delta_cfa + delta_cfb) / (energy_ref * cf_count_scale)
```

The counters are checked for 32-bit signed overflow on every read to avoid large spurious energy jumps.

## Notes

- The component is **not thread-safe for direct reads** when the background task is running — only the owner task may call `energy_metering_read()`. Use `energy_metering_get_latest()` from other tasks.
- Calibration values are hardware-specific. The defaults match the prototype; recalibrate for production boards.
