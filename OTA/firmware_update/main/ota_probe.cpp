#include <cstring>
#include <inttypes.h>
#include "boot_flash_adapter.h"
#include "ota_fetch.h"
#include "esp_console.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#if __has_include("wifi_probe_local.h")
#include "wifi_probe_local.h"
#else
#include "wifi_probe_local.h.example"
#endif

namespace {
constexpr char tag[] = "ota_probe";
EventGroupHandle_t events;
constexpr EventBits_t online = BIT0, offline = BIT1;

void station_event(void *, esp_event_base_t base, int32_t, void *data)
{
    if (base == IP_EVENT) {
        auto *ip = static_cast<ip_event_got_ip_t *>(data);
        ESP_LOGI(tag, "STA IP: " IPSTR, IP2STR(&ip->ip_info.ip));
        xEventGroupClearBits(events, offline);
        xEventGroupSetBits(events, online);
    } else {
        xEventGroupClearBits(events, online);
        xEventGroupSetBits(events, offline);
    }
}

esp_err_t init_station()
{
    events = xEventGroupCreate();
    if (!events) return ESP_ERR_NO_MEM;
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) return err; // Do not erase existing NVS on failure.
    if ((err = esp_netif_init()) != ESP_OK) return err;
    if ((err = esp_event_loop_create_default()) != ESP_OK) return err;
    if (!esp_netif_create_default_wifi_sta()) return ESP_ERR_NO_MEM;
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    if ((err = esp_wifi_init(&init)) != ESP_OK) return err;
    if ((err = esp_wifi_set_storage(WIFI_STORAGE_RAM)) != ESP_OK) return err;
    if ((err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, station_event, nullptr)) != ESP_OK) return err;
    if ((err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, station_event, nullptr)) != ESP_OK) return err;
    wifi_config_t cfg{};
    static_assert(sizeof(PROBE_SSID)-1 <= sizeof(cfg.sta.ssid));
    static_assert(sizeof(PROBE_PASSWORD)-1 <= sizeof(cfg.sta.password));
    std::memcpy(cfg.sta.ssid, PROBE_SSID, sizeof(PROBE_SSID)-1);
    std::memcpy(cfg.sta.password, PROBE_PASSWORD, sizeof(PROBE_PASSWORD)-1);
    if ((err = esp_wifi_set_mode(WIFI_MODE_STA)) != ESP_OK) return err;
    if ((err = esp_wifi_set_config(WIFI_IF_STA, &cfg)) != ESP_OK) return err;
    return esp_wifi_start();
}

} // namespace

bool ota_station_ready()
{
    if (!PROBE_SSID[0]) {
        ESP_LOGE(tag, "Configure wifi_probe_local.h and rebuild Anchor A");
        return false;
    }
    static const esp_err_t init = init_station();
    if (init != ESP_OK) {
        ESP_LOGE(tag, "Station init failed: %s; reset before retry", esp_err_to_name(init));
        return false;
    }
    if (!(xEventGroupGetBits(events) & online)) {
        xEventGroupClearBits(events, offline);
        if (esp_wifi_connect() != ESP_OK || !(xEventGroupWaitBits(events, online | offline,
            pdFALSE, pdFALSE, pdMS_TO_TICKS(20000)) & online)) {
            ESP_LOGE(tag, "Wi-Fi connection failed or no IP within 20 s");
            return false;
        }
    }
    return true;
}

namespace {
int probe(int argc, char **argv)
{
    if (argc != 2 || std::strncmp(argv[1], "http://", 7) != 0) {
        ESP_LOGE(tag, "Usage: ota_probe http://PC-IP:8000/uwb_swap_anchor.bin");
        return 1;
    }
    if (!ota_station_ready()) return 1;
    esp_http_client_config_t cfg{};
    cfg.url = argv[1];
    cfg.method = HTTP_METHOD_HEAD;
    cfg.timeout_ms = 5000;
    cfg.disable_auto_redirect = true;
    auto client = esp_http_client_init(&cfg);
    if (!client) return 1;
    const auto err = esp_http_client_open(client, 0);
    const int64_t length = err == ESP_OK ? esp_http_client_fetch_headers(client) : -1;
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    const bool ok = err == ESP_OK && status == 200 && length > 0 && length <= boot_swap::kLayout[4].size;
    ESP_LOGI(tag, "HEAD %s status=%d bytes=%" PRId64 " open=%s (image not downloaded or verified)",
             ok ? "PASS" : "FAIL", status, length, esp_err_to_name(err));
    return ok ? 0 : 1;
}
}

void register_ota_probe()
{
    esp_console_cmd_t cmd{};
    cmd.command = "ota_probe";
    cmd.help = "Check firmware URL and length on a trusted LAN (HEAD only)";
    cmd.func = probe;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
