#pragma once

#include <stddef.h>
#include <stdint.h>

namespace hal::spi {

enum LogFlag : unsigned int {
    LOG_FLAG_INIT = 1U << 0,
    LOG_FLAG_WRITE_READ = 1U << 1,
};

bool init();
bool write_read(const uint8_t *tx_data, uint8_t *rx_data, size_t size);

}  // namespace hal::spi
