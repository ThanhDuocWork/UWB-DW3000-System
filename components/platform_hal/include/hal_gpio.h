#pragma once

namespace hal::gpio {

enum LogFlag : unsigned int {
    LOG_FLAG_INIT = 1U << 0,
    LOG_FLAG_SET_LEVEL = 1U << 1,
};

bool init();
void set_level(int pin, bool high);

}  // namespace hal::gpio
