#pragma once

#include "board_config.h"
#include "board_pins.h"

namespace board {

enum LogFlag : unsigned int {
    LOG_FLAG_INIT = 1U << 0,
};

void init();
void init_board_uwb();

}  // namespace board
