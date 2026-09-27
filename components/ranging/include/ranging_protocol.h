#pragma once
#include "ranging_measurement.h"
#include "uwb_frame.h"

namespace ranging {

constexpr uint8_t PROTOCOL_VERSION = 1U;
constexpr uint8_t CONTROL_SIZE = 2U;
constexpr uint8_t FINAL_SIZE = 17U;

bool matches_frame(const uwb::Frame &frame, uwb::FrameType type,
                   uint16_t local, uint16_t peer, uint8_t sequence);
bool encode_final(uwb::Frame &frame, const Session &session);
bool decode_final(const uwb::Frame &frame, Timestamps &time);
bool run_initiator(Session &session, uint16_t local, uint16_t peer);
Measurement run_responder(Session &session, uint16_t local);

}  // namespace ranging
