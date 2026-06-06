#include "dw3000_driver.h"

#include "uwb_log.h"

namespace dw3000 {

void dump_diag()
{
    const auto info = get_device_info();
    UWB_LOGI("dw3000_diag", LOG_FLAG_DIAG, "diag placeholder devid=0x%08lx", static_cast<unsigned long>(info.device_id));
}

}  // namespace dw3000
