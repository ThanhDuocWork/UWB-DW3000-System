#include "ranging_protocol.h"
#include "dw3000_driver.h"
#include "uwb_mac.h"
#include "uwb_frame_codec.h"
#include "uwb_log.h"

#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (false)

namespace {
uwb::Frame incoming[2]{};
uint64_t incoming_ts[2]{};
uwb::Frame sent[2]{};
unsigned received_count = 0, sent_count = 0, input_count = 0;
bool fail_tx = false;
uint64_t predicted = 0;
uint64_t response_tx = 0;

void reset_mock()
{
    received_count = sent_count = input_count = 0;
    fail_tx = false;
}

uwb::Frame control(uwb::FrameType type, uint16_t src, uint16_t dst, uint8_t seq)
{
    uwb::Frame f{};
    f.type = type;
    f.src = src;
    f.dst = dst;
    f.payload_size = ranging::CONTROL_SIZE;
    f.payload[0] = ranging::PROTOCOL_VERSION;
    f.payload[1] = seq;
    return f;
}
}  // namespace

extern "C" void uwb_log_write(uwb_log_level_t, uint32_t, const char *,
                              const char *, int, const char *, ...) {}

namespace dw3000 {
bool issue_command(uint32_t) { return true; }
bool predict_delayed_tx_timestamp(uint32_t high, uint64_t *out)
{
    predicted = ((static_cast<uint64_t>(high & 0xFFFFFFFEU) << 8U) + 16385U) &
                ranging::TIME_MASK;
    *out = predicted;
    return true;
}
}

namespace uwb {
bool send(const Frame &frame, const dw3000::TxOptions &options, uint64_t *timestamp)
{
    if (fail_tx || sent_count >= 2) return false;
    sent[sent_count++] = frame;
    if (frame.type == FrameType::Poll) {
        if (!options.response_expected || options.delayed) return false;
        *timestamp = 10000U;
    } else if (frame.type == FrameType::Response) {
        if (!options.response_expected || !options.delayed) return false;
        *timestamp = response_tx;
    } else {
        if (options.response_expected || !options.delayed) return false;
        *timestamp = predicted;
    }
    return true;
}
bool receive(Frame &frame, uint32_t, uint64_t *timestamp, dw3000::RxMode mode)
{
    if (received_count >= input_count) return false;
    const auto expected = incoming[received_count].type == FrameType::Poll
        ? dw3000::RxMode::StartNow : dw3000::RxMode::AfterTx;
    if (mode != expected) return false;
    frame = incoming[received_count];
    *timestamp = incoming_ts[received_count++];
    return true;
}
}

int main()
{
    ranging::Session session{};
    ranging::reset_session(session, 255U, 100U);
    session.time.t1 = 0x0102030405ULL;
    session.time.t4 = ranging::TIME_MASK;
    session.time.t5 = 0U;
    auto final = control(uwb::FrameType::Final, 1U, 100U, 255U);
    CHECK(ranging::encode_final(final, session));
    CHECK(final.payload[2] == 5U && final.payload[6] == 1U);
    ranging::Timestamps decoded{};
    CHECK(ranging::decode_final(final, decoded));
    CHECK(decoded.t1 == session.time.t1 && decoded.t4 == session.time.t4 &&
          decoded.t5 == 0U);
    CHECK(ranging::matches_frame(final, uwb::FrameType::Final, 100U, 1U, 255U));
    CHECK(!ranging::matches_frame(final, uwb::FrameType::Final, 100U, 2U, 255U));
    CHECK(!ranging::matches_frame(final, uwb::FrameType::Final, 100U, 1U, 0U));
    uint8_t wire[48]{};
    CHECK(uwb::encode_frame(final, wire, 10U) == 0U);
    const auto length = uwb::encode_frame(final, wire, sizeof(wire));
    CHECK(length == 22U);
    uwb::Frame parsed{};
    CHECK(uwb::decode_frame(wire, length, parsed));
    CHECK(ranging::decode_final(parsed, decoded));
    CHECK(decoded.t1 == session.time.t1);
    CHECK(!uwb::decode_frame(wire, 48U, parsed));
    --final.payload_size;
    CHECK(!ranging::decode_final(final, decoded));

    reset_mock();
    incoming[0] = control(uwb::FrameType::Response, 100U, 1U, 255U);
    incoming_ts[0] = 3000000000ULL;
    input_count = 1;
    CHECK(ranging::run_initiator(session, 1U, 100U));
    CHECK(session.phase == ranging::Phase::Complete && sent_count == 2);
    CHECK(sent[1].type == uwb::FrameType::Final);
    CHECK(ranging::decode_final(sent[1], decoded));
    CHECK(decoded.t5 == predicted && session.time.t5 == predicted);

    reset_mock();
    CHECK(!ranging::run_initiator(session, 1U, 100U)); // Missing RESPONSE.
    CHECK(session.phase == ranging::Phase::Failed && session.time.t1 == 0U);
    reset_mock();
    input_count = 1;
    incoming[0].payload[1] = 254U; // Wrong sequence.
    CHECK(!ranging::run_initiator(session, 1U, 100U));
    CHECK(sent_count == 1);
    reset_mock();
    fail_tx = true;
    CHECK(!ranging::run_initiator(session, 1U, 100U));

    reset_mock();
    ranging::Session tag{};
    tag.sequence = 0U;
    tag.time.t1 = 10000U;
    tag.time.t4 = tag.time.t1 + 3000002000ULL;
    tag.time.t5 = tag.time.t4 + 2000000000ULL;
    incoming[0] = control(uwb::FrameType::Poll, 1U, 100U, 0U);
    incoming_ts[0] = 70000U;
    response_tx = incoming_ts[0] + 3000000000ULL;
    incoming[1] = control(uwb::FrameType::Final, 1U, 100U, 0U);
    CHECK(ranging::encode_final(incoming[1], tag));
    incoming_ts[1] = response_tx + 2000002000ULL;
    input_count = 2;
    const auto measurement = ranging::run_responder(session, 100U);
    CHECK(measurement.valid && measurement.distance_m > 4.69 &&
          measurement.distance_m < 4.70);
    CHECK(session.phase == ranging::Phase::Complete);

    reset_mock();
    input_count = 1; // Drop FINAL.
    CHECK(!ranging::run_responder(session, 100U).valid);
    CHECK(session.phase == ranging::Phase::Failed && session.time.t3 == 0U);
    reset_mock();
    input_count = 2;
    incoming[1].src = 2U; // Wrong peer.
    CHECK(!ranging::run_responder(session, 100U).valid);
    CHECK(session.phase == ranging::Phase::Failed);
    reset_mock();
    incoming[1].src = 1U;
    input_count = 2; // Recovery after failure.
    CHECK(ranging::run_responder(session, 100U).valid);
    return 0;
}
