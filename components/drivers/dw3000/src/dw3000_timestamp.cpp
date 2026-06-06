#include "hal_timer.h"

#include <stdint.h>

namespace dw3000 {

uint64_t get_local_timestamp_us()
{
    return hal::timer::now_us();
}

}  // namespace dw3000
