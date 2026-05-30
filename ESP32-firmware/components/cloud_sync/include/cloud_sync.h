#ifndef CLOUD_SYNC_H
#define CLOUD_SYNC_H

#include "esp_err.h"
#include <stdbool.h>

#define CLOUD_SYNC_DEFAULT_INTERVAL_MS  30000U
#define CLOUD_SYNC_DEFAULT_RETRY_MS     5000U

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t cloud_sync_init(void);

esp_err_t cloud_sync_start_task(void);

esp_err_t cloud_sync_stop_task(void);

bool cloud_sync_is_gsm_mode(void);

bool cloud_sync_is_time_synced(void);

bool cloud_sync_is_task_running(void);

uint32_t cloud_sync_get_upload_count(void);

esp_err_t cloud_sync_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
