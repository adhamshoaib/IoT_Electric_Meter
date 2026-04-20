idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES i2c_service ads1115 voltage_sensor current_sensor energy_calc wifi http_client nvs_flash lwip esp_netif)
