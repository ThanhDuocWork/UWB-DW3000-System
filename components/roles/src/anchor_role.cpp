#include "anchor_role.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board_config.h"
#include "ranging_service.h"
#include "uwb_log.h"

namespace roles::anchor {

void run()
{
    const auto local = static_cast<uint16_t>(board::config().node_id);
    UWB_LOGI("anchor_role", LOG_FLAG_RUN, "DS-TWR anchor id=%u", local);
    while (true) {
        (void)ranging::run_anchor_once(local);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

}  // namespace roles::anchor
