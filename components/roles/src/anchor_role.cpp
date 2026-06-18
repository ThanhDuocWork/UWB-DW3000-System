#include "anchor_role.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "uwb_log.h"
#include "uwb_mac.h"

namespace roles::anchor {

static const char *TAG = "anchor_role";

void run()
{
    UWB_LOGI(TAG, LOG_FLAG_RUN, "Anchor role loop start");

    while (true) {
        uwb::Frame frame{};
        if (uwb::receive(frame, 1000U)) {
            UWB_LOGI(TAG,
                     LOG_FLAG_RUN,
                     "Frame rx type=%u src=%u dst=%u payload_len=%u",
                     static_cast<unsigned>(frame.type),
                     static_cast<unsigned>(frame.src),
                     static_cast<unsigned>(frame.dst),
                     static_cast<unsigned>(frame.payload_size));

            if (frame.type == uwb::FrameType::Ping) {
                const unsigned seq = frame.payload_size > 0U ? static_cast<unsigned>(frame.payload[0]) : 0U;
                UWB_LOGI(TAG,
                         LOG_FLAG_RUN,
                         "PING received seq=%u src=%u dst=%u",
                         seq,
                         static_cast<unsigned>(frame.src),
                         static_cast<unsigned>(frame.dst));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

}  // namespace roles::anchor
