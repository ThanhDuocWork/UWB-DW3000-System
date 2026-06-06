idf_component_register(
    SRCS
        "src/dw3000_driver.cpp"
        "src/dw3000_port.cpp"
        "src/dw3000_radio.cpp"
        "src/dw3000_timestamp.cpp"
        "src/dw3000_irq.cpp"
        "src/dw3000_diag.cpp"
    INCLUDE_DIRS "include" "vendor"
    REQUIRES platform_hal board common
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
