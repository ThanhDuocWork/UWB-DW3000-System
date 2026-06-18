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

bool receive(Frame &frame, uint32_t timeout_ms)
{
    std::array<uint8_t, 48> buffer{};
    size_t used = 0U;
    if (!dw3000::receive(buffer.data(), buffer.size(), &used, timeout_ms)) {
        return false;
    }

    return decode_frame(buffer.data(), used, frame);
}

}  // namespace uwb
