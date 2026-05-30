idf_component_register(
    SRCS gsm_ppp.c
    INCLUDE_DIRS include
    REQUIRES gsm_driver lwip driver
)
