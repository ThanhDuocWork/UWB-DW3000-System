#pragma once

namespace uwb {

enum LogFlag : unsigned int {
    LOG_FLAG_STARTUP = 1U << 0,
};

void log_startup();

}  // namespace uwb
