#include "tag_role.h"

#include "ranging_service.h"
#include "uwb_log.h"

namespace roles::tag {

void run()
{
    const auto measurement = ranging::run_once();
    UWB_LOGI("tag_role",
             LOG_FLAG_RUN,
             "Tag role placeholder valid=%d distance=%.2f rssi=%.2f",
             measurement.valid ? 1 : 0,
             measurement.distance_m,
             measurement.rssi_dbm);
}

}  // namespace roles::tag
