idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES diagnostics hardware_config machine_controller machine_state modbus settings web_server
)
