#include "hal_gpio.h"

#include "uwb_log.h"

namespace hal::gpio {

static const char *TAG = "hal_gpio";

bool init()
{
    UWB_LOGI(TAG, LOG_FLAG_INIT, "GPIO HAL init placeholder");
    return true;
}

void set_level(int pin, bool high)
{
    UWB_LOGD(TAG, LOG_FLAG_SET_LEVEL, "GPIO set pin=%d level=%d", pin, high ? 1 : 0);
}

}  // namespace hal::gpio
