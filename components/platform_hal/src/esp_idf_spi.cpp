#include "hal_spi.h"

#include "uwb_log.h"

namespace hal::spi {

static const char *TAG = "hal_spi";

bool init()
{
    UWB_LOGI(TAG, LOG_FLAG_INIT, "SPI HAL init placeholder");
    return true;
}

bool write_read(const uint8_t *tx_data, uint8_t *rx_data, size_t size)
{
    (void)tx_data;
    (void)rx_data;
    UWB_LOGD(TAG, LOG_FLAG_WRITE_READ, "SPI transfer placeholder size=%u", static_cast<unsigned>(size));
    return true;
}

}  // namespace hal::spi
