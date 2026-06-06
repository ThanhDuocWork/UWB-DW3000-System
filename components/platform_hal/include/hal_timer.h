#pragma once

#include <stdint.h>

namespace hal::timer {

uint64_t now_us();
void delay_ms(uint32_t delay_ms);

}  // namespace hal::timer
