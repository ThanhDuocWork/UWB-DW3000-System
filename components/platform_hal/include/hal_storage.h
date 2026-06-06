#pragma once

namespace hal::storage {

enum LogFlag : unsigned int {
    LOG_FLAG_INIT = 1U << 0,
};

bool init();

}  // namespace hal::storage
