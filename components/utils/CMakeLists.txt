idf_component_register(
    SRCS
        "src/uwb_log.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
