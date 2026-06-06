#pragma once

#include <stdint.h>

namespace uwb {

using Address = uint16_t;

enum class FrameType : uint8_t {
    Blink = 0,
    Poll,
    Response,
    Final,
};

}  // namespace uwb
