#include "dw3000_port.h"

#include "dw3000_hal.h"

namespace dw3000 {

bool port_init()
{
    return dw3000_hal::init_interface();
}

}  // namespace dw3000
