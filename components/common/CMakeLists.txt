idf_component_register(
    SRCS
        "src/error_code.cpp"
        "src/time_utils.cpp"
    INCLUDE_DIRS "include"
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
