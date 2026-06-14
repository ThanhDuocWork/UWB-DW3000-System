#pragma once

#include <stddef.h>
#include <stdint.h>

namespace base::spi {

enum LogFlag : unsigned int {
    LOG_FLAG_INIT = 1U << 0,
    LOG_FLAG_TRANSFER = 1U << 1,
};

bool init_spi();
bool transfer(const uint8_t *tx_data, uint8_t *rx_data, size_t size);

}  // namespace base::spi
