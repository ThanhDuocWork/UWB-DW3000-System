#include "ranging_service.h"

#include "ranging_protocol.h"
#include "uwb_log.h"

namespace ranging {

static Session s_session{};

void init()
{
    UWB_LOGI("ranging", LOG_FLAG_INIT, "Ranging service init");
}

Measurement run_once()
{
    return perform_placeholder_exchange(s_session);
}

}  // namespace ranging
