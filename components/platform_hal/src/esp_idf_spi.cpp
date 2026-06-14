#include "base_spi.h"

#include "board_pins.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "uwb_log.h"

namespace base::spi {

static const char *TAG = "base_spi";
static spi_device_handle_t s_device = nullptr;
static bool s_initialized = false;
static constexpr int kDw3000InitClockHz = 2 * 1000 * 1000;

bool init_spi()
{
    if (s_initialized) {
        UWB_LOGD(TAG, LOG_FLAG_INIT, "Base SPI already initialized");
        return true;
    }

    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num = BOARD_UWB_SPI_SCLK_GPIO;
    buscfg.mosi_io_num = BOARD_UWB_SPI_MOSI_GPIO;
    buscfg.miso_io_num = BOARD_UWB_SPI_MISO_GPIO;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 1024;

    const esp_err_t bus_ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (bus_ret != ESP_OK) {
        UWB_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(bus_ret));
        return false;
    }

    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = kDw3000InitClockHz;
    devcfg.mode = 0;
    devcfg.spics_io_num = -1;
    devcfg.queue_size = 1;

    const esp_err_t dev_ret = spi_bus_add_device(SPI2_HOST, &devcfg, &s_device);
    if (dev_ret != ESP_OK) {
        UWB_LOGE(TAG, "SPI add device failed: %s", esp_err_to_name(dev_ret));
        spi_bus_free(SPI2_HOST);
        return false;
    }

    s_initialized = true;
    UWB_LOGI(TAG, LOG_FLAG_INIT,
             "Base SPI init host=%d hz=%d sclk=%d miso=%d mosi=%d",
             SPI2_HOST,
             kDw3000InitClockHz,
             BOARD_UWB_SPI_SCLK_GPIO,
             BOARD_UWB_SPI_MISO_GPIO,
             BOARD_UWB_SPI_MOSI_GPIO);
    return true;
}

bool transfer(const uint8_t *tx_data, uint8_t *rx_data, size_t size)
{
    if (!s_initialized || s_device == nullptr) {
        UWB_LOGE(TAG, "SPI transfer requested before init");
        return false;
    }

    if (size == 0U) {
        UWB_LOGD(TAG, LOG_FLAG_TRANSFER, "Base SPI transfer skipped size=0");
        return true;
    }

    spi_transaction_t trans = {};
    trans.length = static_cast<uint32_t>(size * 8U);
    trans.rxlength = (rx_data != nullptr) ? static_cast<uint32_t>(size * 8U) : 0U;
    trans.tx_buffer = tx_data;
    trans.rx_buffer = rx_data;

    const esp_err_t ret = spi_device_transmit(s_device, &trans);
    if (ret != ESP_OK) {
        UWB_LOGE(TAG, "SPI transmit failed: %s", esp_err_to_name(ret));
        return false;
    }

    UWB_LOGD(TAG, LOG_FLAG_TRANSFER, "Base SPI transfer size=%u", static_cast<unsigned>(size));
    return true;
}

}  // namespace base::spi
