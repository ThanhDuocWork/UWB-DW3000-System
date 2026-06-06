idf_component_register(
    SRCS
        "src/uwb_frame.cpp"
        "src/uwb_frame_codec.cpp"
        "src/uwb_mac.cpp"
        "src/uwb_events.cpp"
    INCLUDE_DIRS "include"
    REQUIRES common dw3000
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
