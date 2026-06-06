idf_component_register(SRCS "cloud_sync.c"
                       INCLUDE_DIRS "include"
                       REQUIRES http_client wifi_sta energy_metering gsm_driver gsm_ppp freertos nvs_flash)
