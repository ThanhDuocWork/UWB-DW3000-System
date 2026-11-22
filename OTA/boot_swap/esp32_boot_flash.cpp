#include "boot_flash_adapter.h"

#include "bootloader_flash_priv.h"
#include "esp_err.h"
#include "hal/wdt_hal.h"

namespace boot_swap {
namespace {
void feed_watchdog()
{
    wdt_hal_context_t rtc_wdt_ctx = RWDT_HAL_CONTEXT_DEFAULT();
    wdt_hal_write_protect_disable(&rtc_wdt_ctx);
    wdt_hal_feed(&rtc_wdt_ctx);
    wdt_hal_write_protect_enable(&rtc_wdt_ctx);
}

bool flash_read(uint32_t address, void *data, unsigned size)
{
    if (bootloader_flash_read(address, data, size, false) != ESP_OK) return false;
    feed_watchdog();
    return true;
}

bool flash_write(uint32_t address, const void *data, unsigned size)
{
    if (bootloader_flash_write(address, const_cast<void *>(data), size, false) != ESP_OK)
        return false;
    feed_watchdog();
    return true;
}

bool flash_erase(uint32_t sector)
{
    if (bootloader_flash_erase_sector(sector) != ESP_OK) return false;
    feed_watchdog();
    return true;
}
}  // namespace

FlashOps esp32_boot_flash_ops()
{
    return {flash_read, flash_write, flash_erase};
}
}  // namespace boot_swap
