cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/components/board"
    "${CMAKE_CURRENT_LIST_DIR}/components/platform_hal"
    "${CMAKE_CURRENT_LIST_DIR}/components/drivers/dw3000"
    "${CMAKE_CURRENT_LIST_DIR}/components/uwb"
    "${CMAKE_CURRENT_LIST_DIR}/components/ranging"
    "${CMAKE_CURRENT_LIST_DIR}/components/roles"
    "${CMAKE_CURRENT_LIST_DIR}/components/app"
    "${CMAKE_CURRENT_LIST_DIR}/components/common"
    "${CMAKE_CURRENT_LIST_DIR}/components/utils"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(uwb-positioning-system)
