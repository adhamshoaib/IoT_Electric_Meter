#pragma once

#include "esp_err.h"
#include "gsm_driver.h"

typedef enum {
    GSM_PPP_DISCONNECTED,
    GSM_PPP_CONNECTING,
    GSM_PPP_CONNECTED,
    GSM_PPP_ERROR,
} gsm_ppp_status_t;

esp_err_t gsm_ppp_start(const gsm_gprs_config_t *config, uint32_t timeout_ms);

esp_err_t gsm_ppp_stop(void);

bool gsm_ppp_is_connected(void);
