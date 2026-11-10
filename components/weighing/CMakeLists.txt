idf_component_register(
    SRCS "src/weighing.cpp"
    INCLUDE_DIRS "include"
    REQUIRES common modbus rs485_transport settings hardware_config esp_timer freertos
)
