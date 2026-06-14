#pragma once

#include "base_timer.h"

namespace hal::timer {

inline uint64_t now_us()
{
    return base::timer::now_us();
}

inline void delay_ms(uint32_t delay_ms)
{
    base::timer::delay_ms(delay_ms);
}

}  // namespace hal::timer
