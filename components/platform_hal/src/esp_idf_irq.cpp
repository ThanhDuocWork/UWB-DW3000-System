#include "base_irq.h"

#include "base_gpio.h"
#include "base_timer.h"
#include "board_pins.h"
#include "uwb_log.h"

namespace base::irq {

static const char *TAG = "base_irq";
static bool s_initialized = false;

bool init_irq()
{
    if (s_initialized) {
        UWB_LOGD(TAG, LOG_FLAG_INIT, "Base IRQ already initialized");
        return true;
    }

    s_initialized = true;
    UWB_LOGI(TAG, LOG_FLAG_INIT, "Base IRQ init pin=%d", BOARD_UWB_IRQ_GPIO);
    return true;
}

bool wait_irq(int pin, uint32_t timeout_ms)
{
    const uint64_t deadline = base::timer::now_us() + (static_cast<uint64_t>(timeout_ms) * 1000ULL);

    UWB_LOGD(TAG, LOG_FLAG_WAIT, "Base IRQ wait pin=%d timeout_ms=%u", pin, static_cast<unsigned>(timeout_ms));

    while (base::timer::now_us() < deadline) {
        if (base::gpio::read(pin)) {
            return true;
        }
        base::timer::delay_ms(1);
    }

    return false;
}

}  // namespace base::irq
