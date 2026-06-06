#pragma once

namespace board {

enum class NodeRole {
    Tag,
    Anchor,
};

struct BoardConfig {
    NodeRole default_role = NodeRole::Tag;
    int node_id = 1;
};

const BoardConfig &config();

}  // namespace board
