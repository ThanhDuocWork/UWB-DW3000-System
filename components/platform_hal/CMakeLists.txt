idf_component_register(
    SRCS
        "src/esp_idf_spi.cpp"
        "src/esp_idf_gpio.cpp"
        "src/esp_idf_timer.cpp"
        "src/esp_idf_irq.cpp"
        "src/esp_idf_storage.cpp"
        "src/esp_idf_log.cpp"
    INCLUDE_DIRS "include"
    REQUIRES board utils nvs_flash esp_timer esp_driver_gpio esp_driver_spi freertos
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
