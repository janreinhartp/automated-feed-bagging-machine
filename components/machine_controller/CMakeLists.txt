idf_component_register(
    SRCS "src/machine_controller.cpp"
    INCLUDE_DIRS "include"
    REQUIRES calibration diagnostics feeder_controller hardware_config industrial_io machine_state
             safety_manager servo_controller settings weighing freertos esp_timer
)
