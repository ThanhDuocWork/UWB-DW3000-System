#include "ota_fetch.h"
#include <algorithm>
#include <cstring>
#include "boot_flash_adapter.h"
#include "esp_app_format.h"
#include "esp_console.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "mbedtls/sha256.h"

namespace {
constexpr char tag[] = "ota_fetch";
constexpr auto slot = boot_swap::kLayout[4];
constexpr size_t sector = boot_swap::kSectorSize;

int nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

bool stage(esp_http_client_handle_t client, const esp_partition_t *part,
           size_t length, const unsigned char expected[32])
{
    uint8_t buf[1024];
    size_t offset = 0;
    const uint8_t magic = ESP_IMAGE_HEADER_MAGIC;
    while (offset < length) {
        const int n = esp_http_client_read(client, reinterpret_cast<char *>(buf),
                                          std::min(sizeof(buf), length - offset));
        if (n <= 0) {
            ESP_LOGE(tag, "Transfer interrupted at %u/%u", unsigned(offset), unsigned(length));
            return false;
        }
        size_t skip = 0;
        if (!offset) {
            if (buf[0] != magic) {
                ESP_LOGE(tag, "Not an ESP image; secondary unchanged");
                return false;
            }
            const size_t erase_len = ((length + sector - 1) / sector) * sector;
            if (esp_partition_erase_range(part, 0, erase_len) != ESP_OK) return false;
            skip = 1; // Leave header invalid until the stored bytes match the trusted digest.
        }
        if (size_t(n) > skip && esp_partition_write(part, offset + skip, buf + skip,
                                                     size_t(n) - skip) != ESP_OK) return false;
        offset += size_t(n);
    }
    if (!esp_http_client_is_complete_data_received(client)) return false;

    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    unsigned char digest[32];
    bool ok = mbedtls_sha256_starts(&sha, 0) == 0;
    for (offset = 0; ok && offset < length;) {
        const size_t n = std::min(sizeof(buf), length - offset);
        ok = esp_partition_read(part, offset, buf, n) == ESP_OK;
        if (!offset) buf[0] = magic;
        ok = ok && mbedtls_sha256_update(&sha, buf, n) == 0;
        offset += n;
    }
    ok = ok && mbedtls_sha256_finish(&sha, digest) == 0;
    mbedtls_sha256_free(&sha);
    if (!ok || std::memcmp(expected, digest, sizeof(digest)) != 0) {
        ESP_LOGE(tag, "Secondary readback/SHA-256 failed; header not committed");
        return false;
    }
    if (esp_partition_write(part, 0, &magic, 1) != ESP_OK) return false;
    if (!ota_verify_candidate(length)) {
        ESP_LOGE(tag, "Image/role/version/length rejected; invalidating secondary");
        if (esp_partition_erase_range(part, 0, sector) != ESP_OK)
            ESP_LOGE(tag, "Secondary invalidation failed");
        return false;
    }
    return true;
}

int fetch(int argc, char **argv)
{
    if (argc != 3 || std::strncmp(argv[1], "http://", 7) || std::strlen(argv[2]) != 64) {
        ESP_LOGE(tag, "Usage: ota_fetch http://host/file.bin SHA256");
        return 1;
    }
    unsigned char expected[32];
    for (unsigned i = 0; i < sizeof(expected); ++i) {
        const int hi = nibble(argv[2][2*i]), lo = nibble(argv[2][2*i+1]);
        if (hi < 0 || lo < 0) { ESP_LOGE(tag, "Invalid SHA-256"); return 1; }
        expected[i] = static_cast<unsigned char>(hi * 16 + lo);
    }
    const auto *part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
        static_cast<esp_partition_subtype_t>(slot.subtype), "secondary");
    if (!part || part->address != slot.offset || part->size != slot.size ||
        !ota_staging_allowed()) {
        ESP_LOGE(tag, "Refused: need valid layout, anchor A.1 and healthy Idle journal");
        return 1;
    }
    if (!ota_station_ready()) return 1;
    esp_http_client_config_t cfg{};
    cfg.url = argv[1];
    cfg.method = HTTP_METHOD_GET;
    cfg.timeout_ms = 8000;
    cfg.disable_auto_redirect = true;
    auto client = esp_http_client_init(&cfg);
    if (!client) return 1;
    const auto err = esp_http_client_open(client, 0);
    const auto length = err == ESP_OK ? esp_http_client_fetch_headers(client) : -1;
    const int status = esp_http_client_get_status_code(client);
    const bool headers_ok = status == 200 && length > 0 && length <= part->size &&
                            !esp_http_client_is_chunked_response(client);
    ESP_LOGI(tag, "GET status=%d bytes=%lld open=%s", status,
             static_cast<long long>(length), esp_err_to_name(err));
    const bool ok = headers_ok && stage(client, part, size_t(length), expected);
    esp_http_client_cleanup(client);
    if (!ok) { ESP_LOGE(tag, "Fetch FAIL; no swap requested"); return 1; }
    ESP_LOGI(tag, "Fetch PASS: secondary SHA-256 and anchor B.1 verified; Idle, no swap");
    return 0;
}
}

void register_ota_fetch()
{
    esp_console_cmd_t cmd{};
    cmd.command = "ota_fetch";
    cmd.help = "Stage firmware B from trusted LAN: ota_fetch URL SHA256";
    cmd.func = fetch;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
