#pragma once

#include "base_gpio.h"

namespace hal::gpio {

using LogFlag = base::gpio::LogFlag;

inline bool init_gpio()
{
    return base::gpio::init_gpio();
}

inline bool config_output(int pin, bool initial_high)
{
    return base::gpio::config_output(pin, initial_high);
}

inline bool config_input(int pin)
{
    return base::gpio::config_input(pin);
}

inline void write(int pin, bool high)
{
    base::gpio::write(pin, high);
}

inline bool read(int pin)
{
    return base::gpio::read(pin);
}

}  // namespace hal::gpio
