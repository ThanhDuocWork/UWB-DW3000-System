#include "base_gpio.h"

#include "board_pins.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "uwb_log.h"

namespace base::gpio {

static const char *TAG = "base_gpio";
static bool s_initialized = false;

bool config_output(int pin, bool initial_high)
{
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = 1ULL << pin;
    cfg.mode = GPIO_MODE_OUTPUT;
    cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;

    const esp_err_t ret = gpio_config(&cfg);
    if (ret != ESP_OK) {
        UWB_LOGE(TAG, "GPIO config output failed pin=%d err=%s", pin, esp_err_to_name(ret));
        return false;
    }

    const esp_err_t level_ret = gpio_set_level(static_cast<gpio_num_t>(pin), initial_high ? 1 : 0);
    if (level_ret != ESP_OK) {
        UWB_LOGE(TAG, "GPIO set output level failed pin=%d err=%s", pin, esp_err_to_name(level_ret));
        return false;
    }

    UWB_LOGD(TAG, LOG_FLAG_CONFIG, "Base GPIO config output pin=%d level=%d", pin, initial_high ? 1 : 0);
    return true;
}

bool config_input(int pin)
{
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = 1ULL << pin;
    cfg.mode = GPIO_MODE_INPUT;
    cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;

    const esp_err_t ret = gpio_config(&cfg);
    if (ret != ESP_OK) {
        UWB_LOGE(TAG, "GPIO config input failed pin=%d err=%s", pin, esp_err_to_name(ret));
        return false;
    }

    UWB_LOGD(TAG, LOG_FLAG_CONFIG, "Base GPIO config input pin=%d", pin);
    return true;
}

bool init_gpio()
{
    if (s_initialized) {
        UWB_LOGD(TAG, LOG_FLAG_INIT, "Base GPIO already initialized");
        return true;
    }

    const bool ok =
        config_output(BOARD_UWB_SPI_CS_GPIO, true) &&
        config_output(BOARD_UWB_RST_GPIO, true) &&
        config_output(BOARD_UWB_WAKEUP_GPIO, true) &&
        config_output(BOARD_STATUS_LED_GPIO, false) &&
        config_input(BOARD_UWB_IRQ_GPIO) &&
        config_input(BOARD_UWB_EXTON_GPIO);

    if (!ok) {
        return false;
    }

    s_initialized = true;
    UWB_LOGI(TAG, LOG_FLAG_INIT,
             "Base GPIO init cs=%d rst=%d wakeup=%d irq=%d exton=%d led=%d",
             BOARD_UWB_SPI_CS_GPIO,
             BOARD_UWB_RST_GPIO,
             BOARD_UWB_WAKEUP_GPIO,
             BOARD_UWB_IRQ_GPIO,
             BOARD_UWB_EXTON_GPIO,
             BOARD_STATUS_LED_GPIO);
    return true;
}

void write(int pin, bool high)
{
    const esp_err_t ret = gpio_set_level(static_cast<gpio_num_t>(pin), high ? 1 : 0);
    if (ret != ESP_OK) {
        UWB_LOGE(TAG, "GPIO write failed pin=%d err=%s", pin, esp_err_to_name(ret));
        return;
    }

    UWB_LOGD(TAG, LOG_FLAG_WRITE, "Base GPIO write pin=%d level=%d", pin, high ? 1 : 0);
}

bool read(int pin)
{
    const int level = gpio_get_level(static_cast<gpio_num_t>(pin));
    UWB_LOGD(TAG, LOG_FLAG_READ, "Base GPIO read pin=%d level=%d", pin, level);
    return level != 0;
}

}  // namespace base::gpio
