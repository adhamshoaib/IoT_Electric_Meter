idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES i2c_service ads1115 voltage_sensor current_sensor)
