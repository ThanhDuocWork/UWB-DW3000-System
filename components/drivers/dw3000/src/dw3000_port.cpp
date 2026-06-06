#include "dw3000_port.h"

#include "hal_gpio.h"
#include "hal_irq.h"
#include "hal_spi.h"

namespace dw3000 {

bool port_init()
{
    return hal::spi::init() && hal::gpio::init() && hal::irq::init();
}

}  // namespace dw3000
