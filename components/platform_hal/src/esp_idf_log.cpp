#include "hal_log.h"

#include "uwb_log.h"

namespace hal::log {

void banner(const char *name)
{
    UWB_LOGI("hal_log", LOG_FLAG_BANNER, "==== %s ====", name);
}

}  // namespace hal::log
