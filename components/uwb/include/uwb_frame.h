#pragma once

#include <array>
#include <stdint.h>

#include "uwb_types.h"

namespace uwb {

struct Frame {
    FrameType type = FrameType::Blink;
    Address src = 0;
    Address dst = 0;
    std::array<uint8_t, 32> payload{};
    uint8_t payload_size = 0;
};

}  // namespace uwb
