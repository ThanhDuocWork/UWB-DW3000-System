idf_component_register(
    SRCS
        "src/app.cpp"
        "src/app_context.cpp"
        "src/app_config.cpp"
    INCLUDE_DIRS "include"
    REQUIRES roles board platform_hal ranging uwb dw3000 common utils
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)

