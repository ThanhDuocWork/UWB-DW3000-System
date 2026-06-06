#include "ranging_protocol.h"

#include "uwb_mac.h"

namespace ranging {

Measurement perform_placeholder_exchange(Session &session)
{
    uwb::Frame frame{};
    frame.type = uwb::FrameType::Poll;
    frame.src = 1;
    frame.dst = 2;
    frame.payload_size = 1;
    frame.payload[0] = static_cast<uint8_t>(session.sequence++ & 0xffU);

    Measurement measurement{};
    measurement.valid = uwb::send(frame);
    measurement.distance_m = measurement.valid ? 1.23f : 0.0f;
    measurement.rssi_dbm = measurement.valid ? -70.0f : 0.0f;
    return measurement;
}

}  // namespace ranging
