#pragma once

namespace hal::log {

enum LogFlag : unsigned int {
    LOG_FLAG_BANNER = 1U << 0,
};

void banner(const char *name);

}  // namespace hal::log
