#include "board.h"

#include "uwb_log.h"

namespace board {

static const char *TAG = "board";

void init_board_uwb()
{
    const auto &board_cfg = config();
    UWB_LOGI(TAG,
             LOG_FLAG_INIT,
             "Board init: sclk=%d miso=%d mosi=%d cs=%d irq=%d rst=%d wakeup=%d exton=%d role=%s id=%d",
             BOARD_UWB_SPI_SCLK_GPIO,
             BOARD_UWB_SPI_MISO_GPIO,
             BOARD_UWB_SPI_MOSI_GPIO,
             BOARD_UWB_SPI_CS_GPIO,
             BOARD_UWB_IRQ_GPIO,
             BOARD_UWB_RST_GPIO,
             BOARD_UWB_WAKEUP_GPIO,
             BOARD_UWB_EXTON_GPIO,
             board_cfg.default_role == NodeRole::Tag ? "tag" : "anchor",
             board_cfg.node_id);
}

void init()
{
    init_board_uwb();
}

}  // namespace board
