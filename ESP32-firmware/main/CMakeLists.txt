idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES BL0939 uart_service energy_metering oled_display i2c_service esp_driver_uart esp_driver_gpio esp_driver_i2c nvs_flash wifi_sta led_driver http_client cloud_sync gsm_driver)
