#pragma once

#include <stdint.h>

#include "uwb_frame.h"

namespace uwb {

bool send(const Frame &frame);
bool receive(Frame &frame, uint32_t timeout_ms);

}  // namespace uwb
