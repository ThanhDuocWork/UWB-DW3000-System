#pragma once

#include <stdint.h>

namespace base::timer {

uint64_t now_us();
void delay_ms(uint32_t delay_ms);

}  // namespace base::timer
