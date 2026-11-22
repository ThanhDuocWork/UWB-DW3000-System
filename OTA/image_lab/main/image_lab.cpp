#include <cstring>
#include <inttypes.h>

#include "boot_flash_adapter.h"
#include "image_lab_config.h"
#include "esp_app_desc.h"
#include "esp_app_format.h"
#include "esp_console.h"
#include "esp_flash.h"
#include "esp_flash_encrypt.h"
#include "esp_flash_partitions.h"
#include "esp_image_format.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
constexpr char TAG[] = "image_lab";
constexpr uint32_t PRIMARY_ADDRESS = boot_swap::kLayout[2].offset;
constexpr uint32_t SECONDARY_ADDRESS = boot_swap::kLayout[4].offset;
constexpr uint32_t SLOT_SIZE = boot_swap::kLayout[2].size;
constexpr uint32_t APP_DESC_OFFSET = sizeof(esp_image_header_t) +
                                     sizeof(esp_image_segment_header_t);
constexpr char PROJECT_NAME[] = "uwb_swap_" SWAP_BOARD_ROLE;
enum class JournalDiagnosticPhase { Off, Scan, Commit };
JournalDiagnosticPhase journal_diagnostic_phase = JournalDiagnosticPhase::Off;
unsigned journal_scan_read_count = 0;

bool checked_layout(boot_swap::Partition *parts, char (*labels)[17])
{
    alignas(4) uint8_t raw[ESP_PARTITION_TABLE_MAX_LEN]{};
    uint32_t flash_size = 0;
    if (esp_flash_get_size(nullptr, &flash_size) != ESP_OK ||
        esp_flash_read(nullptr, raw, ESP_PARTITION_TABLE_OFFSET, sizeof(raw)) != ESP_OK)
        return false;
    const auto *table = reinterpret_cast<const esp_partition_info_t *>(raw);
    int count = 0;
    if (esp_partition_table_verify(table, true, &count) != ESP_OK ||
        count != static_cast<int>(sizeof(boot_swap::kLayout) /
                                  sizeof(boot_swap::kLayout[0]))) return false;
    for (int i = 0; i < count; ++i) {
        std::memcpy(labels[i], table[i].label, 16);
        parts[i] = {labels[i], table[i].type, table[i].subtype,
                    table[i].pos.offset, table[i].pos.size, table[i].flags};
    }
    return boot_swap::layout_valid(parts, static_cast<unsigned>(count),
                                   flash_size, esp_flash_encryption_enabled());
}

bool valid_image(uint32_t address, const char *version)
{
    const esp_partition_pos_t position{address, SLOT_SIZE};
    esp_image_metadata_t metadata{};
    const esp_err_t verify_err = esp_image_verify(ESP_IMAGE_VERIFY_SILENT, &position, &metadata);
    const bool metadata_ok = metadata.image_len != 0 && metadata.image_len <= SLOT_SIZE &&
                             metadata.image.chip_id == ESP_CHIP_ID_ESP32;
    ESP_LOGI(TAG, "Image verify address=0x%08" PRIx32 " exit err=%s(0x%x) len=%" PRIu32
             " chip_id=%u metadata_ok=%s", address, esp_err_to_name(verify_err),
             static_cast<unsigned>(verify_err), metadata.image_len, metadata.image.chip_id,
             metadata_ok ? "yes" : "no");
    if (verify_err != ESP_OK || !metadata_ok) return false;
    esp_app_desc_t desc{};
    const esp_err_t desc_err = esp_flash_read(nullptr, &desc, address + APP_DESC_OFFSET, sizeof(desc));
    const bool magic_ok = desc_err == ESP_OK && desc.magic_word == ESP_APP_DESC_MAGIC_WORD;
    const bool project_ok = magic_ok && std::strncmp(desc.project_name, PROJECT_NAME,
                                                       sizeof(desc.project_name)) == 0;
    const bool version_ok = magic_ok && std::strncmp(desc.version, version,
                                                       sizeof(desc.version)) == 0;
    ESP_LOGI(TAG, "Image descriptor address=0x%08" PRIx32 " err=%s(0x%x) magic=%s "
             "project=%.32s project_ok=%s version=%.32s expected=%.32s version_ok=%s",
             address + APP_DESC_OFFSET, esp_err_to_name(desc_err), static_cast<unsigned>(desc_err),
             magic_ok ? "ok" : "bad", desc.project_name, project_ok ? "yes" : "no",
             desc.version, version, version_ok ? "yes" : "no");
    return desc_err == ESP_OK && magic_ok && project_ok && version_ok;
}

bool actual_layout_valid()
{
    boot_swap::Partition parts[sizeof(boot_swap::kLayout) / sizeof(boot_swap::kLayout[0])]{};
    char labels[sizeof(parts) / sizeof(parts[0])][17]{};
    return checked_layout(parts, labels);
}

bool running_is(const char *version)
{
    const auto *desc = esp_app_get_description();
    return desc && desc->magic_word == ESP_APP_DESC_MAGIC_WORD &&
           std::strncmp(desc->project_name, PROJECT_NAME, sizeof(desc->project_name)) == 0 &&
           std::strncmp(desc->version, version, sizeof(desc->version)) == 0;
}

bool journal_address(uint32_t address, unsigned size)
{
    constexpr uint32_t begin = boot_swap::kLayout[6].offset;
    constexpr uint32_t end = boot_swap::kLayout[7].offset +
                             boot_swap::kLayout[7].size;
    return address >= begin && address <= end && size <= end - address;
}

bool app_read(uint32_t address, void *data, unsigned size)
{
    const auto phase = journal_diagnostic_phase;
    if (phase == JournalDiagnosticPhase::Commit)
        ESP_LOGI(TAG, "Journal flash read BEFORE address=0x%08" PRIx32 " size=%u",
                 address, size);
    const esp_err_t err = esp_flash_read(nullptr, data, address, size);
    if (err != ESP_OK)
        ESP_LOGE(TAG, "Journal flash read ERROR phase=%s address=0x%08" PRIx32
                 " size=%u err=%s(0x%x)",
                 phase == JournalDiagnosticPhase::Scan ? "scan" :
                 phase == JournalDiagnosticPhase::Commit ? "commit" : "off",
                 address, size, esp_err_to_name(err), static_cast<unsigned>(err));
    else if (phase == JournalDiagnosticPhase::Commit)
        ESP_LOGI(TAG, "Journal flash read AFTER address=0x%08" PRIx32 " size=%u err=ESP_OK",
                 address, size);
    else if (phase == JournalDiagnosticPhase::Scan && (++journal_scan_read_count % 32U) == 0)
        ESP_LOGI(TAG, "Journal scan progress successful_reads=%u latest_address=0x%08" PRIx32,
                 journal_scan_read_count, address);
    return err == ESP_OK;
}

bool app_write(uint32_t address, const void *data, unsigned size)
{
    const bool in_journal = journal_address(address, size);
    const bool diagnostic = journal_diagnostic_phase == JournalDiagnosticPhase::Commit;
    if (diagnostic)
        ESP_LOGI(TAG, "Journal flash write BEFORE address=0x%08" PRIx32 " size=%u range_ok=%s",
                 address, size, in_journal ? "yes" : "no");
    const esp_err_t err = in_journal ? esp_flash_write(nullptr, data, address, size) : ESP_ERR_INVALID_ARG;
    if (diagnostic)
        ESP_LOGI(TAG, "Journal flash write AFTER address=0x%08" PRIx32 " size=%u err=%s(0x%x)",
                 address, size, esp_err_to_name(err), static_cast<unsigned>(err));
    return in_journal && err == ESP_OK;
}

bool app_erase(uint32_t sector)
{
    const uint32_t address = sector * boot_swap::kSectorSize;
    const bool in_journal = journal_address(address, boot_swap::kSectorSize);
    const bool diagnostic = journal_diagnostic_phase == JournalDiagnosticPhase::Commit;
    if (diagnostic)
        ESP_LOGI(TAG, "Journal flash erase BEFORE sector=%" PRIu32 " address=0x%08" PRIx32
                 " size=%u range_ok=%s", sector, address, boot_swap::kSectorSize,
                 in_journal ? "yes" : "no");
    const esp_err_t err = in_journal ? esp_flash_erase_region(nullptr, address, boot_swap::kSectorSize)
                                     : ESP_ERR_INVALID_ARG;
    if (diagnostic)
        ESP_LOGI(TAG, "Journal flash erase AFTER sector=%" PRIu32 " address=0x%08" PRIx32
                 " size=%u err=%s(0x%x)", sector, address, boot_swap::kSectorSize,
                 esp_err_to_name(err), static_cast<unsigned>(err));
    return in_journal && err == ESP_OK;
}

void log_console_stack(const char *boundary)
{
    ESP_LOGI(TAG, "Console task stack high-water at %s: %u bytes",
             boundary, static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr) * sizeof(StackType_t)));
}

boot_swap::BootCore &app_core()
{
    // Commands are serialized by the console; all mutations recheck real layout.
    static boot_swap::BootFlashAdapter flash(
        boot_swap::kLayout, sizeof(boot_swap::kLayout) / sizeof(boot_swap::kLayout[0]),
        boot_swap::kFlashSize, false, {app_read, app_write, app_erase});
    static boot_swap::BootCore core(flash);
    return core;
}

int swap_command(int, char **)
{
    if (std::strcmp(SWAP_VARIANT, "A") != 0) {
        ESP_LOGE(TAG, "Swap is only requested from firmware A");
        return 1;
    }
    auto &core = app_core();
    if (!actual_layout_valid() || !running_is("A.1") ||
        !valid_image(PRIMARY_ADDRESS, "A.1") || !valid_image(SECONDARY_ADDRESS, "B.1") ||
        !core.load() ||
        core.mode() != boot_swap::BootCore::Mode::Idle ||
        !core.request_update()) {
        ESP_LOGE(TAG, "Swap request FAIL");
        return 1;
    }
    ESP_LOGW(TAG, "Swap request committed; resetting into bootloader");
    esp_restart();
    return 0;
}

int confirm_command(int, char **)
{
    ESP_LOGI(TAG, "Confirm checkpoint: entry variant=%s", SWAP_VARIANT);
    log_console_stack("confirm entry");
    if (std::strcmp(SWAP_VARIANT, "B") != 0) {
        ESP_LOGE(TAG, "Confirm is only available from firmware B");
        return 1;
    }
    auto &core = app_core();
    ESP_LOGI(TAG, "Confirm checkpoint: layout entry");
    const bool layout_ok = actual_layout_valid();
    ESP_LOGI(TAG, "Confirm checkpoint: layout %s", layout_ok ? "PASS" : "FAIL");
    log_console_stack("after layout");
    const bool running_ok = running_is("B.1");
    const auto *running = esp_app_get_description();
    ESP_LOGI(TAG, "Confirm checkpoint: running descriptor %s project=%.32s version=%.32s",
             running_ok ? "PASS" : "FAIL", running ? running->project_name : "<null>",
             running ? running->version : "<null>");
    ESP_LOGI(TAG, "Confirm checkpoint: image verification entry address=0x%08" PRIx32,
             PRIMARY_ADDRESS);
    const bool image_ok = valid_image(PRIMARY_ADDRESS, "B.1");
    ESP_LOGI(TAG, "Confirm checkpoint: image verification %s", image_ok ? "PASS" : "FAIL");
    log_console_stack("after image verification");
    if (!layout_ok || !running_ok || !image_ok) {
        ESP_LOGE(TAG, "Confirm FAIL: layout, running descriptor, or image preflight failed");
        return 1;
    }
    ESP_LOGI(TAG, "Confirm checkpoint: journal load entry (scan success progress every 32 reads)");
    journal_scan_read_count = 0;
    journal_diagnostic_phase = JournalDiagnosticPhase::Scan;
    const auto journal_status = core.load_status();
    journal_diagnostic_phase = JournalDiagnosticPhase::Off;
    const bool loaded = journal_status == boot_swap::BootCore::JournalStatus::Valid;
    const char *status_name = journal_status == boot_swap::BootCore::JournalStatus::Valid ? "Valid" :
                              journal_status == boot_swap::BootCore::JournalStatus::Blank ? "Blank" :
                              journal_status == boot_swap::BootCore::JournalStatus::Corrupt ? "Corrupt" :
                              "IoError";
    const auto mode = core.mode();
    ESP_LOGI(TAG, "Confirm checkpoint: journal load exit status=%s (%s)", status_name,
             loaded ? "PASS" : "FAIL");
    ESP_LOGI(TAG, "Confirm checkpoint: journal mode=%s", mode == boot_swap::BootCore::Mode::Trial ?
             "Trial" : mode == boot_swap::BootCore::Mode::Idle ? "Idle" :
             mode == boot_swap::BootCore::Mode::Forward ? "Forward" :
             mode == boot_swap::BootCore::Mode::Ready ? "Ready" :
             mode == boot_swap::BootCore::Mode::Reverse ? "Reverse" : "unknown");
    if (!loaded || mode != boot_swap::BootCore::Mode::Trial) {
        ESP_LOGE(TAG, "Confirm FAIL: firmware is not in Trial");
        return 1;
    }
    ESP_LOGI(TAG, "Confirm checkpoint: confirm entry");
    log_console_stack("before core.confirm");
    journal_diagnostic_phase = JournalDiagnosticPhase::Commit;
    const bool confirmed = core.confirm();
    journal_diagnostic_phase = JournalDiagnosticPhase::Off;
    ESP_LOGI(TAG, "Confirm checkpoint: confirm result=%s", confirmed ? "PASS" : "FAIL");
    log_console_stack("after core.confirm");
    if (!confirmed) {
        ESP_LOGE(TAG, "Confirm FAIL: firmware is not in Trial");
        return 1;
    }
    ESP_LOGI(TAG, "Firmware B confirmed; swap is now Idle");
    return 0;
}

void log_slot(const char *label, uint32_t address)
{
    esp_app_desc_t desc{};
    if (!app_read(address + APP_DESC_OFFSET, &desc, sizeof(desc)) ||
        desc.magic_word != ESP_APP_DESC_MAGIC_WORD) {
        ESP_LOGI(TAG, "%s descriptor unavailable (slot may be unstaged)", label);
        return;
    }
    ESP_LOGI(TAG, "%s descriptor project=%.32s version=%.32s (not image verification)",
             label, desc.project_name, desc.version);
}

int provision_command(int argc, char **argv)
{
    if (argc != 2 || std::strcmp(argv[1], "ERASE") != 0) {
        ESP_LOGE(TAG, "Usage: provision ERASE (only on pristine journals)");
        return 1;
    }
    if (std::strcmp(SWAP_VARIANT, "A") != 0) {
        ESP_LOGE(TAG, "Provision only from image A");
        return 1;
    }
    if (!actual_layout_valid() || !running_is("A.1") ||
        !valid_image(PRIMARY_ADDRESS, "A.1") || !valid_image(SECONDARY_ADDRESS, "B.1")) {
        ESP_LOGE(TAG, "Provision refused: layout or image A/B preflight FAIL");
        return 1;
    }
    auto &core = app_core();
    if (!core.provision() || !core.load() ||
        core.mode() != boot_swap::BootCore::Mode::Idle) {
        ESP_LOGE(TAG, "Provision FAIL: journals must be blank; no automatic repair");
        return 1;
    }
    ESP_LOGI(TAG, "Journal provision PASS: Idle; swap command available");
    return 0;
}
}  // namespace

extern "C" void app_main()
{
    const esp_app_desc_t *running = esp_app_get_description();
    ESP_LOGI(TAG, "BOOT role=%s variant=%s project=%s version=%s reset_reason=%d",
             SWAP_BOARD_ROLE, SWAP_VARIANT, running->project_name, running->version,
             static_cast<int>(esp_reset_reason()));
    log_slot("Primary", PRIMARY_ADDRESS);
    log_slot("Secondary", SECONDARY_ADDRESS);
    ESP_LOGI(TAG, "Boot swap enabled; image/layout preflight runs at each command");
    esp_console_cmd_t cmd{};
    cmd.command = "provision";
    cmd.help = "Explicitly provision blank swap journal after A/B preflight: provision ERASE";
    cmd.func = provision_command;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    cmd = {};
    if (std::strcmp(SWAP_VARIANT, "A") == 0) {
        cmd.command = "swap";
        cmd.help = "Request A/B swap and reset into bootloader";
        cmd.func = swap_command;
    } else {
        cmd.command = "confirm";
        cmd.help = "Confirm Trial firmware B";
        cmd.func = confirm_command;
    }
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "image-lab>";
    repl_config.task_stack_size = 16384;
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    esp_console_repl_t *repl = nullptr;
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
