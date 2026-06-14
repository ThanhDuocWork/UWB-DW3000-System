#pragma once

#include <stddef.h>
#include <stdint.h>

namespace dw3000_hal {

enum LogFlag : unsigned int {
    LOG_FLAG_INIT_INTERFACE = 1U << 0,
    LOG_FLAG_RESET = 1U << 1,
    LOG_FLAG_CHIP_SELECT = 1U << 2,
    LOG_FLAG_SPI_READ = 1U << 3,
    LOG_FLAG_SPI_WRITE = 1U << 4,
    LOG_FLAG_WAIT_IRQ = 1U << 5,
    LOG_FLAG_TIME = 1U << 6,
};

bool init_interface();
bool reset_chip();
void select_chip();
void deselect_chip();
bool spi_write(const uint8_t *tx_data, size_t size);
bool spi_read(const uint8_t *tx_data, uint8_t *rx_data, size_t size);
bool wait_irq(uint32_t timeout_ms);
uint64_t now_us();
void delay_ms(uint32_t delay_ms);

}  // namespace dw3000_hal
