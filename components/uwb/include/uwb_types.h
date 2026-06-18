#pragma once

#include <stdint.h>

namespace uwb {

using Address = uint16_t;

enum class FrameType : uint8_t {
    Ping = 0,
    Blink,
    Poll,
    Response,
    Final,
};

}  // namespace uwb
