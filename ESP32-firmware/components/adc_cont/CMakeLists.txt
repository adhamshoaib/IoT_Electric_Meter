idf_component_register(SRCS "adc_cont.c" 
                       INCLUDE_DIRS "."
                       REQUIRES esp_adc esp_adc freertos)