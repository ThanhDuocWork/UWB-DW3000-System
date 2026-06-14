#include "board_config.h"

#include "sdkconfig.h"

namespace board {

const BoardConfig &config()
{
    static const BoardConfig kConfig{
#if CONFIG_UWB_ROLE_ANCHOR
        .default_role = NodeRole::Anchor,
#else
        .default_role = NodeRole::Tag,
#endif
        .node_id = CONFIG_UWB_NODE_ID,
    };
    return kConfig;
}

}  // namespace board
