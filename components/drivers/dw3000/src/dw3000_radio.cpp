#include "dw3000_driver.h"

#include "uwb_log.h"

namespace dw3000 {

bool transmit(const uint8_t *data, size_t size)
{
    (void)data;
    UWB_LOGI("dw3000_radio", LOG_FLAG_TX, "TX placeholder size=%u", static_cast<unsigned>(size));
    return size > 0;
}

bool start_receive()
{
    UWB_LOGI("dw3000_radio", LOG_FLAG_RX, "RX enable placeholder");
    return true;
}

}  // namespace dw3000
