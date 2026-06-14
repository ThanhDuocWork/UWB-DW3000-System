#include "dw3000_hal.h"

#include <stdint.h>

namespace dw3000 {

uint64_t get_local_timestamp_us()
{
    return dw3000_hal::now_us();
}

}  // namespace dw3000
