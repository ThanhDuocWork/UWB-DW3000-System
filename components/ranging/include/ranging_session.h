#pragma once
#include <stdint.h>

namespace ranging {

constexpr uint64_t TIME_MASK = 0xFFFFFFFFFFULL;
constexpr uint64_t UUS_TO_DTU = 65536ULL;
constexpr uint32_t REPLY_DELAY_UUS = 50000U;
constexpr uint32_t RX_TIMEOUT_UUS = 100000U;
constexpr uint32_t HOST_TIMEOUT_MS = 300U;

enum class Phase { Idle, WaitResponse, SendFinal, WaitFinal, Complete, Failed };

struct Timestamps {
    uint64_t t1 = 0, t2 = 0, t3 = 0, t4 = 0, t5 = 0, t6 = 0;
};

struct Session {
    uint8_t sequence = 0;
    uint16_t peer = 0;
    Phase phase = Phase::Idle;
    Timestamps time{};
};

void reset_session(Session &session, uint8_t sequence, uint16_t peer);
void fail_session(Session &session);

}  // namespace ranging
