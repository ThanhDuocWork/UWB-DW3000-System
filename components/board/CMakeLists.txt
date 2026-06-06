idf_component_register(
    SRCS
        "src/board.cpp"
        "src/board_config.cpp"
    INCLUDE_DIRS "include"
    REQUIRES common
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
