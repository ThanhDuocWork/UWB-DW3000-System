#include "dw3000_hal.h"

#include "base_gpio.h"
#include "base_irq.h"
#include "base_spi.h"
#include "base_timer.h"
#include "board_pins.h"
#include "uwb_log.h"

namespace dw3000_hal {

static const char *TAG = "dw3000_hal";

bool init_interface()
{
    UWB_LOGI(TAG, LOG_FLAG_INIT_INTERFACE, "DW3000 HAL init interface");
    if (!base::gpio::init_gpio() || !base::spi::init_spi() || !base::irq::init_irq()) {
        return false;
    }

    base::gpio::write(BOARD_UWB_WAKEUP_GPIO, true);
    base::gpio::write(BOARD_UWB_SPI_CS_GPIO, true);
    base::timer::delay_ms(5);
    return true;
}

bool reset_chip()
{
    UWB_LOGI(TAG, LOG_FLAG_RESET, "DW3000 HAL reset chip");
    base::gpio::write(BOARD_UWB_WAKEUP_GPIO, true);
    base::gpio::write(BOARD_UWB_SPI_CS_GPIO, true);
    base::timer::delay_ms(2);
    base::gpio::write(BOARD_UWB_RST_GPIO, false);
    base::timer::delay_ms(5);
    base::gpio::write(BOARD_UWB_RST_GPIO, true);
    base::timer::delay_ms(10);
    return true;
}

void select_chip()
{
    UWB_LOGD(TAG, LOG_FLAG_CHIP_SELECT, "DW3000 HAL select chip");
    base::gpio::write(BOARD_UWB_SPI_CS_GPIO, false);
}

void deselect_chip()
{
    UWB_LOGD(TAG, LOG_FLAG_CHIP_SELECT, "DW3000 HAL deselect chip");
    base::gpio::write(BOARD_UWB_SPI_CS_GPIO, true);
}

bool spi_write(const uint8_t *tx_data, size_t size)
{
    UWB_LOGD(TAG, LOG_FLAG_SPI_WRITE, "DW3000 HAL SPI write size=%u", (unsigned)size);
    select_chip();
    const bool ok = base::spi::transfer(tx_data, nullptr, size);
    deselect_chip();
    return ok;
}

bool spi_read(const uint8_t *tx_data, uint8_t *rx_data, size_t size)
{
    UWB_LOGD(TAG, LOG_FLAG_SPI_READ, "DW3000 HAL SPI read size=%u", (unsigned)size);
    select_chip();
    const bool ok = base::spi::transfer(tx_data, rx_data, size);
    deselect_chip();
    return ok;
}

bool wait_irq(uint32_t timeout_ms)
{
    UWB_LOGD(TAG, LOG_FLAG_WAIT_IRQ, "DW3000 HAL wait IRQ timeout_ms=%u", (unsigned)timeout_ms);
    return base::irq::wait_irq(BOARD_UWB_IRQ_GPIO, timeout_ms);
}

uint64_t now_us()
{
    UWB_LOGD(TAG, LOG_FLAG_TIME, "DW3000 HAL read time");
    return base::timer::now_us();
}

void delay_ms(uint32_t delay_ms)
{
    base::timer::delay_ms(delay_ms);
}

}  // namespace dw3000_hal
