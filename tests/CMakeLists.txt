cmake_minimum_required(VERSION 3.16)
project(uwb_host_tests LANGUAGES CXX)
enable_testing()
set(CMAKE_CXX_STANDARD 17)

set(ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
add_executable(ranging_math_test
    ranging_math_test.cpp
    ${ROOT}/components/ranging/src/ranging_measurement.cpp
    ${ROOT}/components/ranging/src/ranging_session.cpp)
target_include_directories(ranging_math_test PRIVATE ${ROOT}/components/ranging/include)
add_test(NAME ranging_math COMMAND ranging_math_test)

add_executable(ranging_protocol_test
    ranging_protocol_test.cpp
    ${ROOT}/components/ranging/src/ranging_protocol.cpp
    ${ROOT}/components/ranging/src/ranging_measurement.cpp
    ${ROOT}/components/ranging/src/ranging_session.cpp
    ${ROOT}/components/uwb/src/uwb_frame_codec.cpp)
target_include_directories(ranging_protocol_test PRIVATE
    ${ROOT}/components/ranging/include
    ${ROOT}/components/uwb/include
    ${ROOT}/components/drivers/dw3000/include
    ${ROOT}/components/utils/include)
add_test(NAME ranging_protocol COMMAND ranging_protocol_test)
