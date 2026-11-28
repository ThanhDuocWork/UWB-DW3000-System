set(update_sources firmware_update.cpp)
if(SWAP_BOARD_ROLE STREQUAL "anchor" AND SWAP_VARIANT STREQUAL "B")
    list(APPEND update_sources anchor_radio.cpp)
endif()
if(SWAP_BOARD_ROLE STREQUAL "anchor" AND SWAP_VARIANT STREQUAL "A")
    list(APPEND update_sources ota_probe.cpp ota_fetch.cpp)
endif()
idf_component_register(SRCS ${update_sources}
    INCLUDE_DIRS "../../boot_swap"
    PRIV_REQUIRES esp_app_format bootloader_support esp_partition spi_flash log console
                  esp_wifi esp_netif esp_event esp_http_client freertos nvs_flash mbedtls)
if(SWAP_BOARD_ROLE STREQUAL "anchor" AND SWAP_VARIANT STREQUAL "A")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ENABLE_OTA_PROBE=1)
endif()
configure_file("firmware_update_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/firmware_update_config.h" @ONLY)
target_include_directories(${COMPONENT_LIB} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
if(SWAP_BOARD_ROLE STREQUAL "anchor" AND SWAP_VARIANT STREQUAL "B")
    target_link_libraries(${COMPONENT_LIB} PRIVATE idf::board idf::ranging idf::uwb idf::dw3000)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ENABLE_ANCHOR_RADIO=1)
endif()
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
