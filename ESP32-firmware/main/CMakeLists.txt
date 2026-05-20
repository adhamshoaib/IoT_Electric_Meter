idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES BL0939 uart_service energy_metering esp_driver_uart esp_driver_gpio)
