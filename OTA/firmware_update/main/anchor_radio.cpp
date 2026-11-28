#include "anchor_radio.h"
#include <atomic>
#include "board.h"
#include "board_config.h"
#include "dw3000_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ranging_protocol.h"
#include "ranging_service.h"
#include "uwb_mac.h"

namespace anchor_radio {
namespace {
constexpr char tag[] = "anchor_radio";
constexpr uint16_t expected_tag = 1;
std::atomic<bool> rx_passed{false};

void run(void *)
{
    board::init();
    const auto local = static_cast<uint16_t>(board::config().node_id);
    if (!dw3000::init(dw3000::Config{}) || dw3000::read_device_id() != 0xdeca0302U) {
        ESP_LOGE(tag, "DW3000 bring-up failed; confirmation blocked");
        vTaskDelete(nullptr);
        return;
    }
    ranging::init();
    ESP_LOGI(tag, "DW3000 ready; waiting for Tag %u at Anchor %u", unsigned(expected_tag), unsigned(local));
    for (;;) {
        uwb::Frame frame{};
        if (uwb::receive(frame, 1000U) && frame.src == expected_tag) {
            const bool ping = frame.type == uwb::FrameType::Ping &&
                              (frame.dst == local || frame.dst == 0xffffU);
            const bool poll = ranging::matches_frame(frame, uwb::FrameType::Poll,
                                                     local, expected_tag, frame.payload[1]);
            if (ping || poll) {
                rx_passed.store(true);
                ESP_LOGI(tag, "UWB RX PASS: %s src=%u dst=%u; confirm permitted",
                         ping ? "PING" : "POLL", unsigned(frame.src), unsigned(frame.dst));
                break;
            }
        }
        vTaskDelay(1);
    }
    // The first frame proves RX; respond to subsequent ranging exchanges normally.
    for (;;) {
        (void)ranging::run_anchor_once(local);
        vTaskDelay(1);
    }
}
}

bool received_from_tag() { return rx_passed.load(); }

void start()
{
    if (xTaskCreate(run, "anchor_radio", 8192, nullptr, 5, nullptr) != pdPASS)
        ESP_LOGE(tag, "Cannot start radio task; confirmation blocked");
}
}
