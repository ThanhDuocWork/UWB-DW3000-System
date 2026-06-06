#include "dw3000_driver.h"

#include "dw3000_port.h"
#include "uwb_log.h"

namespace dw3000 {

static const char *TAG = "dw3000";
static DeviceInfo s_info{};

bool init(const Config &config)
{
    (void)config;

    if (!port_init()) {
        s_info.state = State::Error;
        return false;
    }

    s_info.device_id = 0xDECA0302U;
    s_info.state = State::Ready;
    UWB_LOGI(TAG, LOG_FLAG_INIT, "DW3000 init placeholder, devid=0x%08lx", static_cast<unsigned long>(s_info.device_id));
    return true;
}

DeviceInfo get_device_info()
{
    return s_info;
}

}  // namespace dw3000
