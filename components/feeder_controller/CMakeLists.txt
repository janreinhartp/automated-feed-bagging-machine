idf_component_register(
    SRCS "src/feeder_controller.cpp"
    INCLUDE_DIRS "include"
    REQUIRES common hardware_config esp_driver_ledc esp_driver_gpio
)
