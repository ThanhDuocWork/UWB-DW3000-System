idf_component_register(
    SRCS
        "src/anchor_role.cpp"
        "src/tag_role.cpp"
    INCLUDE_DIRS "include"
    REQUIRES ranging board common
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
