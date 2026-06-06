#pragma once

#include "board_config.h"

namespace app {

struct Config {
    board::NodeRole role = board::NodeRole::Tag;
    int node_id = 1;
};

Config load_config();

}  // namespace app
