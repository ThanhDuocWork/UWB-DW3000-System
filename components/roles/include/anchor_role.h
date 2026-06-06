#pragma once

namespace roles::anchor {

enum LogFlag : unsigned int {
    LOG_FLAG_RUN = 1U << 0,
};

void run();

}  // namespace roles::anchor
