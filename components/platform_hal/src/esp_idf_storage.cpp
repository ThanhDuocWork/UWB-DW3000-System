#include "hal_storage.h"

#include "nvs_flash.h"

#include "uwb_log.h"

namespace hal::storage {

static const char *TAG = "hal_storage";

bool init()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    if (ret != ESP_OK) {
        UWB_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(ret));
        return false;
    }

    UWB_LOGI(TAG, LOG_FLAG_INIT, "Storage HAL init done");
    return true;
}

}  // namespace hal::storage
