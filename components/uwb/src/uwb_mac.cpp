#include "uwb_mac.h"

#include <array>

#include "dw3000_driver.h"
#include "uwb_frame_codec.h"

namespace uwb {

bool send(const Frame &frame)
{
    std::array<uint8_t, 48> buffer{};
    const size_t used = encode_frame(frame, buffer.data(), buffer.size());
    return used > 0 && dw3000::transmit(buffer.data(), used);
}

}  // namespace uwb
