idf_component_register(SRCS "image_lab.cpp"
    INCLUDE_DIRS "../../boot_swap"
    PRIV_REQUIRES esp_app_format bootloader_support esp_partition spi_flash log console)
configure_file("image_lab_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/image_lab_config.h" @ONLY)
target_include_directories(${COMPONENT_LIB} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
