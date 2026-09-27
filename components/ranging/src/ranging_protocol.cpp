#include "ranging_protocol.h"

#include "dw3000_driver.h"
#include "dw3000_registers.h"
#include "ranging_service.h"
#include "uwb_log.h"
#include "uwb_mac.h"

namespace ranging {

namespace {

constexpr const char *TAG = "ranging";

bool abort_exchange(Session &session, const char *reason)
{
    UWB_LOGE(TAG, "DS-TWR abort seq=%u peer=%u phase=%u reason=%s",
             static_cast<unsigned>(session.sequence), static_cast<unsigned>(session.peer),
             static_cast<unsigned>(session.phase), reason);
    (void)dw3000::issue_command(dw3000::registers::CMD_TXRXOFF);
    fail_session(session);
    return false;
}

uwb::Frame control_frame(uwb::FrameType type, uint16_t local, const Session &session)
{
    uwb::Frame frame{};
    frame.type = type;
    frame.src = local;
    frame.dst = session.peer;
    frame.payload_size = CONTROL_SIZE;
    frame.payload[0] = PROTOCOL_VERSION;
    frame.payload[1] = session.sequence;
    return frame;
}

dw3000::TxOptions wait_for_reply()
{
    dw3000::TxOptions options{};
    options.response_expected = true;
    options.rx_after_tx_uus = 500U;
    options.rx_timeout_uus = RX_TIMEOUT_UUS;
    return options;
}

}  // namespace

bool matches_frame(const uwb::Frame &frame, uwb::FrameType type,
                   uint16_t local, uint16_t peer, uint8_t sequence)
{
    const uint8_t size = type == uwb::FrameType::Final ? FINAL_SIZE : CONTROL_SIZE;
    return frame.type == type && frame.src == peer && frame.dst == local &&
           frame.payload_size == size && frame.payload[0] == PROTOCOL_VERSION &&
           frame.payload[1] == sequence;
}

bool encode_final(uwb::Frame &frame, const Session &session)
{
    const uint64_t values[] = {session.time.t1, session.time.t4, session.time.t5};
    for (const auto value : values) {
        if (value > TIME_MASK) {
            return false;
        }
    }
    frame.type = uwb::FrameType::Final;
    frame.payload_size = FINAL_SIZE;
    frame.payload[0] = PROTOCOL_VERSION;
    frame.payload[1] = session.sequence;
    for (unsigned field = 0; field < 3; ++field) {
        for (unsigned byte = 0; byte < 5; ++byte) {
            frame.payload[2 + field * 5 + byte] =
                static_cast<uint8_t>(values[field] >> (byte * 8U));
        }
    }
    return true;
}

bool decode_final(const uwb::Frame &frame, Timestamps &time)
{
    if (frame.type != uwb::FrameType::Final || frame.payload_size != FINAL_SIZE ||
        frame.payload[0] != PROTOCOL_VERSION) {
        return false;
    }
    uint64_t values[3] = {};
    for (unsigned field = 0; field < 3; ++field) {
        for (unsigned byte = 0; byte < 5; ++byte) {
            values[field] |= static_cast<uint64_t>(frame.payload[2 + field * 5 + byte])
                             << (byte * 8U);
        }
    }
    time.t1 = values[0];
    time.t4 = values[1];
    time.t5 = values[2];
    return true;
}

bool run_initiator(Session &session, uint16_t local, uint16_t peer)
{
    reset_session(session, session.sequence, peer);
    if (local == 0U || local == 0xFFFFU || peer == 0U || peer == 0xFFFFU || peer == local) {
        return abort_exchange(session, "invalid address");
    }
    session.phase = Phase::WaitResponse;
    const uwb::Frame poll = control_frame(uwb::FrameType::Poll, local, session);
    if (!uwb::send(poll, wait_for_reply(), &session.time.t1)) {
        return abort_exchange(session, "POLL TX");
    }
    uwb::Frame response{};
    if (!uwb::receive(response, HOST_TIMEOUT_MS, &session.time.t4, dw3000::RxMode::AfterTx)) {
        return abort_exchange(session, "RESPONSE timeout/RX");
    }
    if (!matches_frame(response, uwb::FrameType::Response, local, peer, session.sequence)) {
        return abort_exchange(session, "RESPONSE peer/seq/format");
    }

    session.phase = Phase::SendFinal;
    dw3000::TxOptions final_tx{};
    final_tx.delayed = true;
    final_tx.delayed_time_high32 = static_cast<uint32_t>(
        ((session.time.t4 + REPLY_DELAY_UUS * UUS_TO_DTU) & TIME_MASK) >> 8U);
    if (!dw3000::predict_delayed_tx_timestamp(final_tx.delayed_time_high32, &session.time.t5)) {
        return abort_exchange(session, "FINAL timestamp prediction");
    }
    uwb::Frame final = control_frame(uwb::FrameType::Final, local, session);
    uint64_t actual_t5 = 0U;
    if (!encode_final(final, session) || !uwb::send(final, final_tx, &actual_t5) ||
        actual_t5 != session.time.t5) {
        return abort_exchange(session, "FINAL delayed TX/timestamp");
    }
    session.phase = Phase::Complete;
    UWB_LOGI(TAG, LOG_FLAG_EXCHANGE,
             "FINAL sent seq=%u peer=%u t1=0x%010llx t4=0x%010llx t5=0x%010llx verified=1",
             static_cast<unsigned>(session.sequence), static_cast<unsigned>(peer),
             static_cast<unsigned long long>(session.time.t1),
             static_cast<unsigned long long>(session.time.t4),
             static_cast<unsigned long long>(session.time.t5));
    // This confirms local TX, not receipt or measurement at the anchor.
    return true;
}

Measurement run_responder(Session &session, uint16_t local)
{
    reset_session(session, 0U, 0U);
    uwb::Frame poll{};
    if (!uwb::receive(poll, 1000U, &session.time.t2)) {
        fail_session(session);
        return {};
    }
    if (poll.payload_size != CONTROL_SIZE || poll.src == 0U ||
        poll.src == 0xFFFFU || poll.src == local ||
        !matches_frame(poll, uwb::FrameType::Poll, local, poll.src, poll.payload[1])) {
        (void)abort_exchange(session, "POLL address/format");
        return {};
    }
    session.peer = poll.src;
    session.sequence = poll.payload[1];
    session.phase = Phase::WaitFinal;

    auto response_tx = wait_for_reply();
    response_tx.delayed = true;
    response_tx.delayed_time_high32 = static_cast<uint32_t>(
        ((session.time.t2 + REPLY_DELAY_UUS * UUS_TO_DTU) & TIME_MASK) >> 8U);
    const uwb::Frame response = control_frame(uwb::FrameType::Response, local, session);
    if (!uwb::send(response, response_tx, &session.time.t3)) {
        (void)abort_exchange(session, "RESPONSE delayed TX");
        return {};
    }
    uwb::Frame final{};
    if (!uwb::receive(final, HOST_TIMEOUT_MS, &session.time.t6, dw3000::RxMode::AfterTx)) {
        (void)abort_exchange(session, "FINAL timeout/RX");
        return {};
    }
    if (!matches_frame(final, uwb::FrameType::Final, local, session.peer, session.sequence) ||
        !decode_final(final, session.time)) {
        (void)abort_exchange(session, "FINAL peer/seq/format");
        return {};
    }

    const auto &t = session.time;
    UWB_LOGI(TAG, LOG_FLAG_EXCHANGE,
             "DS-TWR seq=%u peer=%u t1=0x%010llx t4=0x%010llx t5=0x%010llx",
             static_cast<unsigned>(session.sequence), static_cast<unsigned>(session.peer),
             static_cast<unsigned long long>(t.t1), static_cast<unsigned long long>(t.t4),
             static_cast<unsigned long long>(t.t5));
    UWB_LOGI(TAG, LOG_FLAG_EXCHANGE,
             "DS-TWR seq=%u peer=%u t2=0x%010llx t3=0x%010llx t6=0x%010llx",
             static_cast<unsigned>(session.sequence), static_cast<unsigned>(session.peer),
             static_cast<unsigned long long>(t.t2), static_cast<unsigned long long>(t.t3),
             static_cast<unsigned long long>(t.t6));
    const Measurement result = calculate_measurement(t);
    if (!result.valid) {
        UWB_LOGE(TAG, "RANGE rejected seq=%u tof_dtu=%.3f distance_m=%.3f",
                 static_cast<unsigned>(session.sequence), result.tof_dtu, result.distance_m);
        (void)abort_exchange(session, "invalid intervals/ToF/range");
        return result;
    }
    session.phase = Phase::Complete;
    UWB_LOGI(TAG, LOG_FLAG_EXCHANGE, "RANGE seq=%u peer=%u distance_m=%.3f tof_dtu=%.3f valid=1",
             static_cast<unsigned>(session.sequence), static_cast<unsigned>(session.peer),
             result.distance_m, result.tof_dtu);
    return result;
}

}  // namespace ranging
