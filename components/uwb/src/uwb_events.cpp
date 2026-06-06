#include "uwb_events.h"

#include "uwb_log.h"

namespace uwb {

void log_startup()
{
    UWB_LOGI("uwb", LOG_FLAG_STARTUP, "UWB stack startup");
}

}  // namespace uwb
