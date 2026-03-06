#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "esp_http_client.h"
#include "esp_log.h"

#include <string.h>
esp_err_t firebase_put_last(time_t ts, float energy_kwh);

#endif