#pragma once

namespace app {

enum LogFlag : unsigned int {
    LOG_FLAG_START = 1U << 0,
};

void start();

}  // namespace app
