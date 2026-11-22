cmake_minimum_required(VERSION 3.16)

set(SWAP_BOARD_ROLE "anchor" CACHE STRING "anchor or tag")
set(SWAP_VARIANT "A" CACHE STRING "A or B")
if(NOT SWAP_BOARD_ROLE MATCHES "^(anchor|tag)$")
    message(FATAL_ERROR "SWAP_BOARD_ROLE must be anchor or tag")
endif()
if(NOT SWAP_VARIANT MATCHES "^(A|B)$")
    message(FATAL_ERROR "SWAP_VARIANT must be A or B")
endif()

set(PROJECT_VER "${SWAP_VARIANT}.1")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(uwb_swap_${SWAP_BOARD_ROLE})
