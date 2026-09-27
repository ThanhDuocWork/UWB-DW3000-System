#pragma once
#include "ranging_measurement.h"

namespace ranging {

enum LogFlag : unsigned int {
    LOG_FLAG_INIT = 1U << 0,
    LOG_FLAG_EXCHANGE = 1U << 1,
};

void init();
bool run_tag_once(uint16_t local, uint16_t peer);
Measurement run_anchor_once(uint16_t local);

}  // namespace ranging
