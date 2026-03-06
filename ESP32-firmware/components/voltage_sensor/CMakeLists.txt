idf_component_register(SRCS "voltage_sensor.c"
                       INCLUDE_DIRS "."
                       REQUIRES freertos adc_cont)