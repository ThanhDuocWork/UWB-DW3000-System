#include "board.h"

#include "uwb_log.h"

namespace board {

static const char *TAG = "board";

void init()
{
    const auto &pin_cfg = pins();
    const auto &board_cfg = config();
    UWB_LOGI(TAG,
             LOG_FLAG_INIT,
             "Board init: sck=%d mosi=%d miso=%d cs=%d rst=%d irq=%d role=%s id=%d",
             pin_cfg.spi_sck,
             pin_cfg.spi_mosi,
             pin_cfg.spi_miso,
             pin_cfg.uwb_cs,
             pin_cfg.uwb_rst,
             pin_cfg.uwb_irq,
             board_cfg.default_role == NodeRole::Tag ? "tag" : "anchor",
             board_cfg.node_id);
}

}  // namespace board
