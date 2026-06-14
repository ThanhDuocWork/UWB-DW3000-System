#pragma once

namespace base::gpio {

enum LogFlag : unsigned int {
    LOG_FLAG_INIT = 1U << 0,
    LOG_FLAG_CONFIG = 1U << 1,
    LOG_FLAG_WRITE = 1U << 2,
    LOG_FLAG_READ = 1U << 3,
};

bool init_gpio();
bool config_output(int pin, bool initial_high);
bool config_input(int pin);
void write(int pin, bool high);
bool read(int pin);

}  // namespace base::gpio
