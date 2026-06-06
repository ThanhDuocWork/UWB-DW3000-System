idf_component_register(
    SRCS
        "src/ranging_service.cpp"
        "src/ranging_protocol.cpp"
        "src/ranging_session.cpp"
        "src/ranging_measurement.cpp"
    INCLUDE_DIRS "include"
    REQUIRES common uwb
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
