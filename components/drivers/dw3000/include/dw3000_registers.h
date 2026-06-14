#pragma once

#include <stddef.h>
#include <stdint.h>

namespace dw3000::registers {

constexpr uint16_t NO_SUB_ADDRESS = 0xFFFFU;
constexpr uint16_t DEV_ID = 0x00U;
constexpr size_t DEV_ID_LEN = 4U;

}  // namespace dw3000::registers
