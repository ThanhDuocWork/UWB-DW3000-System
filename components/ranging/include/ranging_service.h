#pragma once

#include "ranging_measurement.h"

namespace ranging {

enum LogFlag : unsigned int {
    LOG_FLAG_INIT = 1U << 0,
};

void init();
Measurement run_once();

}  // namespace ranging
