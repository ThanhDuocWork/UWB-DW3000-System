#pragma once

#include <stdint.h>

namespace base::irq {

enum LogFlag : unsigned int {
    LOG_FLAG_INIT = 1U << 0,
    LOG_FLAG_WAIT = 1U << 1,
};

bool init_irq();
bool wait_irq(int pin, uint32_t timeout_ms);

}  // namespace base::irq
