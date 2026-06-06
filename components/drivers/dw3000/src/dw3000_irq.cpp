#include "dw3000_driver.h"
#include "uwb_log.h"

namespace dw3000 {

void irq_poll_once()
{
    UWB_LOGD("dw3000_irq", LOG_FLAG_IRQ_POLL, "IRQ poll placeholder");
}

}  // namespace dw3000
