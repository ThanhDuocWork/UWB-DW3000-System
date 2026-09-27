#include "ranging_service.h"
#include "ranging_protocol.h"
#include "uwb_log.h"

namespace ranging {

static Session s_session{};
static uint8_t s_sequence = 0U;

void init()
{
    s_session = {};
    s_sequence = 0U;
    UWB_LOGI("ranging", LOG_FLAG_INIT, "DS-TWR ready: POLL/RESPONSE/FINAL reply_uus=%lu",
             static_cast<unsigned long>(REPLY_DELAY_UUS));
}

bool run_tag_once(uint16_t local, uint16_t peer)
{
    reset_session(s_session, s_sequence++, peer);
    return run_initiator(s_session, local, peer);
}

Measurement run_anchor_once(uint16_t local)
{
    reset_session(s_session, 0U, 0U);
    return run_responder(s_session, local);
}

}  // namespace ranging
