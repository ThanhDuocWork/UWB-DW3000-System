#include "ranging_session.h"

namespace ranging {

void reset_session(Session &session, uint8_t sequence, uint16_t peer)
{
    session = {};
    session.sequence = sequence;
    session.peer = peer;
}

void fail_session(Session &session)
{
    session.time = {};
    session.phase = Phase::Failed;
}

}  // namespace ranging
