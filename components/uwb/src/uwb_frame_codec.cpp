#include "uwb_frame_codec.h"

namespace uwb {

size_t encode_frame(const Frame &frame, uint8_t *buffer, size_t buffer_size)
{
    if (buffer == nullptr || buffer_size < 5 || frame.payload_size > frame.payload.size()) {
        return 0;
    }

    buffer[0] = static_cast<uint8_t>(frame.type);
    buffer[1] = static_cast<uint8_t>(frame.src & 0xffU);
    buffer[2] = static_cast<uint8_t>((frame.src >> 8) & 0xffU);
    buffer[3] = static_cast<uint8_t>(frame.dst & 0xffU);
    buffer[4] = static_cast<uint8_t>((frame.dst >> 8) & 0xffU);

    size_t total = 5;
    for (uint8_t i = 0; i < frame.payload_size && total < buffer_size; ++i) {
        buffer[total++] = frame.payload[i];
    }

    return total;
}

bool decode_frame(const uint8_t *buffer, size_t size, Frame &frame)
{
    if (buffer == nullptr || size < 5) {
        return false;
    }

    frame.type = static_cast<FrameType>(buffer[0]);
    frame.src = static_cast<Address>(buffer[1] | (buffer[2] << 8));
    frame.dst = static_cast<Address>(buffer[3] | (buffer[4] << 8));
    frame.payload_size = 0;

    for (size_t i = 5; i < size && frame.payload_size < frame.payload.size(); ++i) {
        frame.payload[frame.payload_size++] = buffer[i];
    }

    return true;
}

}  // namespace uwb
