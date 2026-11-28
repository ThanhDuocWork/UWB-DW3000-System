idf_component_register(SRCS "boot_swap_hook.cpp" "../../../boot_swap/esp32_boot_flash.cpp"
    INCLUDE_DIRS "../../../boot_swap"
    PRIV_REQUIRES bootloader_support esp_app_format spi_flash hal)
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
