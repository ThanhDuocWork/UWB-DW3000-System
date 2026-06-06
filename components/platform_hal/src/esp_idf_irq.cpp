#include "hal_irq.h"

#include "uwb_log.h"

namespace hal::irq {

static const char *TAG = "hal_irq";

bool init()
{
    UWB_LOGI(TAG, LOG_FLAG_INIT, "IRQ HAL init placeholder");
    return true;
}

}  // namespace hal::irq
