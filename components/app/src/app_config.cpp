#include "app_config.h"

#include "board_config.h"

namespace app {

Config load_config()
{
    const auto &board_cfg = board::config();
    return Config{
        .role = board_cfg.default_role,
        .node_id = board_cfg.node_id,
    };
}

}  // namespace app
