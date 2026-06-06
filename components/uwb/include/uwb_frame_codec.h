#pragma once

#include <stddef.h>
#include <stdint.h>

#include "uwb_frame.h"

namespace uwb {

size_t encode_frame(const Frame &frame, uint8_t *buffer, size_t buffer_size);
bool decode_frame(const uint8_t *buffer, size_t size, Frame &frame);

}  // namespace uwb
