idf_component_register(
    SRCS "src/rs485_transport.cpp"
    INCLUDE_DIRS "include"
    REQUIRES common esp_driver_uart
)
