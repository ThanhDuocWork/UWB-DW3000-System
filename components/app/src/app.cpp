#include "app.h"

#include "anchor_role.h"
#include "app_config.h"
#include "app_context.h"
#include "board.h"
#include "dw3000_driver.h"
#include "hal_log.h"
#include "hal_storage.h"
#include "ranging_service.h"
#include "tag_role.h"
#include "uwb_events.h"
#include "uwb_log.h"

namespace app {

static const char *TAG = "app";

void start()
{
    auto &ctx = context();
    hal::log::banner("uwb-positioning-system");

    board::init();
    hal::storage::init();

    ctx.config = load_config();
    ctx.initialized = true;

    uwb::log_startup();
    ranging::init();
    dw3000::init({});

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
