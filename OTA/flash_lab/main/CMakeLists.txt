idf_component_register(SRCS "flash_lab.cpp"
    INCLUDE_DIRS "../../boot_swap"
    PRIV_REQUIRES console esp_partition spi_flash esp_system bootloader_support freertos log)
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
