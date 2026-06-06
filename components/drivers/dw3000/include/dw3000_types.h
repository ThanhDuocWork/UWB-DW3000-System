#pragma once

#include <stdint.h>

namespace dw3000 {

enum class State {
    Reset,
    Ready,
    Rx,
    Tx,
    Error,
};

struct DeviceInfo {
    uint32_t device_id = 0;
    State state = State::Reset;
};

}  // namespace dw3000
