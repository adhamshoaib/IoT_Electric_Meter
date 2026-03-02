// wifi.h — only API
#ifndef WIFI_H
#define WIFI_H
#include <stdbool.h>

void wifi_init(void);
bool wifi_is_connected(void);
void wifi_wait_connected(void);

#endif