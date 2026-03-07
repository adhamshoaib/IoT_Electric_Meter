idf_component_register(SRCS "current_sensor.c"
                       INCLUDE_DIRS "."
                       REQUIRES freertos adc_cont)