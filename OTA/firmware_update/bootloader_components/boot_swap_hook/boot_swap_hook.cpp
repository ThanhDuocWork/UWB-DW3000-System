#include <cstring>
#include <new>

#include "boot_flash_adapter.h"
#include "boot_flow.h"
#include "bootloader_flash_priv.h"
#include "bootloader_utility.h"
#include "esp_app_desc.h"
#include "esp_app_format.h"
#include "esp_flash_encrypt.h"
#include "esp_flash_partitions.h"
#include "esp_image_format.h"
#include "esp_log.h"
#include "esp_rom_spiflash.h"

namespace {
constexpr char TAG[] = "swap_hook";
// Exactly one core: its two sector buffers must not consume the boot stack.
alignas(boot_swap::BootCore) unsigned char core_storage[sizeof(boot_swap::BootCore)];
alignas(boot_swap::BootFlashAdapter) unsigned char flash_storage[sizeof(boot_swap::BootFlashAdapter)];
constexpr uint32_t kAppDescOffset =
    sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t);
bool read_flash(uint32_t address, void *data, unsigned size)
{
    return bootloader_flash_read(address, data, size, false) == ESP_OK;
}

bool verify_image(const boot_swap::Partition &part, const char *expected_version,
                  const char *expected_project, char *project_out)
{
    const esp_partition_pos_t position{part.offset, part.size};
    esp_image_metadata_t metadata{};
    const esp_err_t err = esp_image_verify(ESP_IMAGE_VERIFY_SILENT, &position, &metadata);
    if (err != ESP_OK || metadata.image_len == 0 ||
        metadata.image_len > part.size || metadata.image.chip_id != ESP_CHIP_ID_ESP32) {
        ESP_LOGE(TAG, "%s image invalid: err=%d len=%lu", part.label,
                 static_cast<int>(err), static_cast<unsigned long>(metadata.image_len));
        return false;
    }
    alignas(4) esp_app_desc_t desc{};
    if (!read_flash(part.offset + kAppDescOffset, &desc, sizeof(desc)) ||
        desc.magic_word != ESP_APP_DESC_MAGIC_WORD ||
        (expected_project &&
         std::strncmp(desc.project_name, expected_project,
                      sizeof(desc.project_name)) != 0) ||
        (!expected_project &&
         std::strncmp(desc.project_name, "uwb_swap_anchor",
                      sizeof(desc.project_name)) != 0 &&
         std::strncmp(desc.project_name, "uwb_swap_tag",
                      sizeof(desc.project_name)) != 0) ||
        (expected_version ?
         std::strncmp(desc.version, expected_version, sizeof(desc.version)) != 0 :
         std::strncmp(desc.version, "A.1", sizeof(desc.version)) != 0 &&
         std::strncmp(desc.version, "B.1", sizeof(desc.version)) != 0)) {
        ESP_LOGE(TAG, "%s role/version mismatch", part.label);
        return false;
    }
    ESP_LOGI(TAG, "%s image PASS version=%s len=%lu", part.label,
             desc.version, static_cast<unsigned long>(metadata.image_len));
    if (project_out) std::memcpy(project_out, desc.project_name,
                                  sizeof(desc.project_name));
    return true;
}

bool inspect()
{
    alignas(4) uint8_t raw[ESP_PARTITION_TABLE_MAX_LEN]{};
    if (!read_flash(ESP_PARTITION_TABLE_OFFSET, raw, sizeof(raw))) {
        ESP_LOGE(TAG, "partition table read FAIL");
        return false;
    }
    const auto *table = reinterpret_cast<const esp_partition_info_t *>(raw);
    int count = 0;
    if (esp_partition_table_verify(table, true, &count) != ESP_OK ||
        count != static_cast<int>(sizeof(boot_swap::kLayout) /
                                  sizeof(boot_swap::kLayout[0]))) {
        ESP_LOGE(TAG, "partition table verification/count FAIL");
        return false;
    }
    bool layout_ok = g_rom_flashchip.chip_size == boot_swap::kFlashSize &&
                     !esp_flash_encryption_enabled();
    for (unsigned i = 0; i < sizeof(boot_swap::kLayout) /
                                sizeof(boot_swap::kLayout[0]); ++i) {
        const auto &expected = boot_swap::kLayout[i];
        const auto &actual = table[i];
        layout_ok = layout_ok &&
                    std::strncmp(reinterpret_cast<const char *>(actual.label),
                                 expected.label, sizeof(actual.label)) == 0 &&
                    actual.type == expected.type &&
                    actual.subtype == expected.subtype &&
                    actual.pos.offset == expected.offset &&
                    actual.pos.size == expected.size &&
                    actual.flags == expected.flags;
    }
    if (!layout_ok) {
        ESP_LOGE(TAG, "4 MB layout/encryption FAIL");
        return false;
    }
    ESP_LOGI(TAG, "partition table and 4 MB layout PASS");
    auto *flash = new (flash_storage) boot_swap::BootFlashAdapter(
        boot_swap::kLayout, sizeof(boot_swap::kLayout) / sizeof(boot_swap::kLayout[0]),
        boot_swap::kFlashSize, false, boot_swap::esp32_boot_flash_ops());
    auto *core = new (core_storage) boot_swap::BootCore(*flash);
    if (!flash->ready()) return false;
    boot_swap::BootCore::Mode mode{};
    uint32_t sequence = 0;
    const auto status = boot_swap::BootCore::inspect_status(*flash, mode, sequence);
    using Status = boot_swap::BootCore::JournalStatus;
    const char *status_name = status == Status::Valid ? "Valid" :
                              status == Status::Blank ? "Blank" :
                              status == Status::Corrupt ? "Corrupt" : "IoError";
    ESP_LOGI(TAG, "journal status=%s mode=%lu seq=%lu (mode valid only for Valid)",
             status_name, static_cast<unsigned long>(mode), static_cast<unsigned long>(sequence));
    if (status == Status::Corrupt || status == Status::IoError) return false;
    const bool ok = boot_swap::boot_flow(*core, [](boot_swap::PrimaryVersion version) {
        const char *expected = version == boot_swap::PrimaryVersion::A ? "A.1" :
                               version == boot_swap::PrimaryVersion::B ? "B.1" : nullptr;
        return verify_image(boot_swap::kLayout[2], expected, nullptr, nullptr);
    });
    if (ok) {
        const char *outcome = status == Status::Blank ? "commissioning A.1; provision required" :
            core->mode() == boot_swap::BootCore::Mode::Trial ? "Trial" : "Idle";
        ESP_LOGI(TAG, "boot policy PASS: %s", outcome);
    }
    return ok;
}
}  // namespace

extern "C" void bootloader_hooks_include(void) {}

extern "C" void bootloader_after_init(void)
{
    if (!inspect()) {
        ESP_LOGE(TAG, "boot policy FAIL; resetting without executing Primary");
        bootloader_reset();
    }
}
