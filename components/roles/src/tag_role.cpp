#include "tag_role.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_config.h"
#include "uwb_log.h"
#include "uwb_mac.h"

namespace roles::tag {

static const char *TAG = "tag_role";

namespace {

constexpr uwb::Address kBroadcastAddress = 0xFFFFU;
constexpr TickType_t kPingPeriodTicks = pdMS_TO_TICKS(1000);

}  // namespace

void run()
{
    const auto &board_cfg = board::config();
    uint8_t seq = 0U;

    UWB_LOGI(TAG, LOG_FLAG_RUN, "Tag role loop start node_id=%d", board_cfg.node_id);

    while (true) {
        uwb::Frame frame{};
        frame.type = uwb::FrameType::Ping;
        frame.src = static_cast<uwb::Address>(board_cfg.node_id & 0xFFFF);
        frame.dst = kBroadcastAddress;
        frame.payload[0] = seq;
        frame.payload[1] = 'P';
        frame.payload[2] = 'I';
        frame.payload[3] = 'N';
        frame.payload[4] = 'G';
        frame.payload_size = 5U;

        if (uwb::send(frame)) {
            UWB_LOGI(TAG,
                     LOG_FLAG_RUN,
                     "PING sent seq=%u src=%u dst=%u",
                     static_cast<unsigned>(seq),
                     static_cast<unsigned>(frame.src),
                     static_cast<unsigned>(frame.dst));
        } else {
            UWB_LOGE(TAG, "PING send failed seq=%u", static_cast<unsigned>(seq));
        }

        ++seq;
        vTaskDelay(kPingPeriodTicks);
    }
}

}  // namespace roles::tag

