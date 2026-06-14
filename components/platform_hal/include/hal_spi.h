#pragma once

#include "base_spi.h"

namespace hal::spi {

using LogFlag = base::spi::LogFlag;

inline bool init_spi()
{
    return base::spi::init_spi();
}

inline bool transfer(const uint8_t *tx_data, uint8_t *rx_data, size_t size)
{
    return base::spi::transfer(tx_data, rx_data, size);
}

}  // namespace hal::spi
