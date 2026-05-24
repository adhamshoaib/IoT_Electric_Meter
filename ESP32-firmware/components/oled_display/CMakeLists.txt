idf_component_register(SRCS "oled_display.c"
                       INCLUDE_DIRS "include"
                       REQUIRES i2c_service freertos)
