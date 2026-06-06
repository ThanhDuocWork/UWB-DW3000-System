#pragma once

namespace hal::irq {

enum LogFlag : unsigned int {
    LOG_FLAG_INIT = 1U << 0,
};

bool init();

}  // namespace hal::irq
