#include "hal_timer.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace hal::timer {

uint64_t now_us()
{
    return static_cast<uint64_t>(esp_timer_get_time());
}

void delay_ms(uint32_t delay_ms)
{
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
}

}  // namespace hal::timer
