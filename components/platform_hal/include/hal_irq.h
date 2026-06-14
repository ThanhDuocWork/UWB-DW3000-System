#pragma once

#include "base_irq.h"

namespace hal::irq {

using LogFlag = base::irq::LogFlag;

inline bool init_irq()
{
    return base::irq::init_irq();
}

inline bool wait_irq(int pin, uint32_t timeout_ms)
{
    return base::irq::wait_irq(pin, timeout_ms);
}

}  // namespace hal::irq
