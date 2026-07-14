#include "app.h"

#include "anchor_role.h"
#include "app_config.h"
#include "app_context.h"
#include "base_gpio.h"
#include "base_irq.h"
#include "base_spi.h"
#include "board.h"
#include "dw3000_driver.h"
#include "dw3000_hal.h"
#include "hal_log.h"
#include "hal_storage.h"
#include "ranging_service.h"
#include "tag_role.h"
#include "uwb_events.h"
#include "uwb_log.h"

namespace app {

static const char *TAG = "app";

static void enable_bringup_logs()
{
    UWB_LOG_SET_FLAGS(TAG, LOG_FLAG_START);
    UWB_LOG_SET_FLAGS("board", board::LOG_FLAG_INIT);
    UWB_LOG_SET_FLAGS("base_spi", base::spi::LOG_FLAG_INIT | base::spi::LOG_FLAG_TRANSFER);
    UWB_LOG_SET_FLAGS("base_gpio", base::gpio::LOG_FLAG_INIT | base::gpio::LOG_FLAG_CONFIG | base::gpio::LOG_FLAG_WRITE);
    UWB_LOG_SET_FLAGS("base_irq", base::irq::LOG_FLAG_INIT);
    UWB_LOG_SET_FLAGS("dw3000_hal",
                      dw3000_hal::LOG_FLAG_INIT_INTERFACE |
                          dw3000_hal::LOG_FLAG_RESET |
                          dw3000_hal::LOG_FLAG_SPI_READ |
                          dw3000_hal::LOG_FLAG_SPI_WRITE);
    UWB_LOG_SET_FLAGS("dw3000",
                      dw3000::LOG_FLAG_INIT |
                          dw3000::LOG_FLAG_REG |
                          dw3000::LOG_FLAG_TX |
                          dw3000::LOG_FLAG_RX |
                          dw3000::LOG_FLAG_DIAG);
    UWB_LOG_SET_FLAGS("uwb", uwb::LOG_FLAG_STARTUP);
    UWB_LOG_SET_FLAGS("ranging", ranging::LOG_FLAG_INIT);
    UWB_LOG_SET_FLAGS("anchor_role", roles::anchor::LOG_FLAG_RUN);
    UWB_LOG_SET_FLAGS("tag_role", roles::tag::LOG_FLAG_RUN);
}

void start()
{
    auto &ctx = context();

    enable_bringup_logs();
    hal::log::banner("uwb-positioning-system");

    board::init();
    hal::storage::init();

    ctx.config = load_config();
    ctx.initialized = true;

    uwb::log_startup();
    ranging::init();

    if (!dw3000::init({})) {
        UWB_LOGE(TAG, "DW3000 bring-up failed, stop app start");
        return;
    }

    UWB_LOGI(TAG,
             LOG_FLAG_START,
             "App start role=%s node_id=%d",
             ctx.config.role == board::NodeRole::Tag ? "tag" : "anchor",
             ctx.config.node_id);

    if (ctx.config.role == board::NodeRole::Anchor) {
        roles::anchor::run();
    } else {
        roles::tag::run();
    }
}

}  // namespace app
