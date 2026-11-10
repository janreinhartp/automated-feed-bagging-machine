idf_component_register(
    SRCS "src/settings.cpp" "src/settings_model.cpp"
    INCLUDE_DIRS "include"
    REQUIRES common nvs_flash
)
