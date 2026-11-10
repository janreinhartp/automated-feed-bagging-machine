idf_component_register(
    SRCS "src/modbus_crc.cpp" "src/modbus_master.cpp"
    INCLUDE_DIRS "include"
    REQUIRES common rs485_transport hardware_config
)
