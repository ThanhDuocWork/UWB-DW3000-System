#include "tag_role.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board_config.h"
#include "ranging_service.h"
#include "uwb_log.h"
#include "sdkconfig.h"

namespace roles::tag {

void run()
{
    const auto local = static_cast<uint16_t>(board::config().node_id);
#ifdef CONFIG_UWB_PEER_ID
    constexpr uint16_t peer = CONFIG_UWB_PEER_ID;
#else
    constexpr uint16_t peer = 100U;
#endif
    UWB_LOGI("tag_role", LOG_FLAG_RUN, "DS-TWR tag id=%u anchor=%u", local, peer);
    while (true) {
        (void)ranging::run_tag_once(local, peer);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

}  // namespace roles::tag

