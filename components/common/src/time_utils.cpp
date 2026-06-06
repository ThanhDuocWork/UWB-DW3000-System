#include "time_utils.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace common {

uint64_t millis()
{
    return static_cast<uint64_t>(xTaskGetTickCount()) * portTICK_PERIOD_MS;
}

void sleep_ms(uint32_t delay_ms)
{
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
}

}  // namespace common
