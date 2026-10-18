#include "swap_core.h"

#include <cstdio>
#include <cstring>
#include "esp_console.h"
#include "esp_flash.h"
#include "esp_flash_encrypt.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
constexpr char TAG[] = "swap_lab";
constexpr unsigned SECTOR = 4096;
constexpr unsigned SLOT_SECTORS = 3;
constexpr unsigned LAB_ADDRESS = 0x3f0000;
using Engine = boot_swap::Core<SECTOR, SLOT_SECTORS>;
const esp_partition_t *partition = nullptr;
bool boot_ok = false;
// Static buffers: do not consume the app_main or console task stack.
std::array<uint8_t, SECTOR> check_buffer{};

bool flash_result(esp_err_t err, const char *operation)
{
    if (err != ESP_OK) ESP_LOGE(TAG, "%s failed: %s", operation, esp_err_to_name(err));
    return err == ESP_OK;
}

class LabFlash final : public Engine::Flash {
public:
    bool read(unsigned s, unsigned o, void *data, unsigned size) override
    {
        return bounded(s, o, size) &&
               flash_result(esp_partition_read(partition, s * SECTOR + o, data, size), "read");
    }
    bool erase(unsigned s) override
    {
        if (!bounded(s, 0, SECTOR)) return false;
        ESP_LOGI(TAG, "ERASE sector=%u", s);
        if (!flash_result(esp_partition_erase_range(partition, s * SECTOR, SECTOR), "erase"))
            return false;
        pause();
        return true;
    }
    bool write(unsigned s, unsigned o, const void *data, unsigned size) override
    {
        if (!bounded(s, o, size)) return false;
        const auto *bytes = static_cast<const uint8_t *>(data);
        // Small writes and delays expose checkpoints for manual power-cut tests.
        // This is intentionally slow and is not a production flash adapter.
        for (unsigned done = 0; done < size;) {
            const unsigned chunk = size - done > 256 ? 256 : size - done;
            if (!flash_result(esp_partition_write(partition, s * SECTOR + o + done,
                                                  bytes + done, chunk), "write")) return false;
            ESP_LOGI(TAG, "PROGRAM sector=%u offset=%u bytes=%u", s, o + done, chunk);
            pause();
            done += chunk;
        }
        return true;
    }
private:
    static bool bounded(unsigned s, unsigned o, unsigned size)
    {
        return partition && s < Engine::sector_count && o <= SECTOR && size <= SECTOR - o;
    }
    static void pause() { vTaskDelay(pdMS_TO_TICKS(50)); }
};

LabFlash flash;
Engine engine(flash);

uint8_t pattern(char image, unsigned offset)
{
    return static_cast<uint8_t>(image == 'A' ? offset * 17U + 3U : offset * 31U + 7U);
}

bool matches(unsigned first_sector, char image)
{
    for (unsigned s = 0; s < SLOT_SECTORS; ++s) {
        if (!flash.read(first_sector + s, 0, check_buffer.data(), SECTOR)) return false;
        for (unsigned i = 0; i < SECTOR; ++i)
            if (check_buffer[i] != pattern(image, s * SECTOR + i)) return false;
    }
    return true;
}

char identify(unsigned first_sector)
{
    if (matches(first_sector, 'A')) return 'A';
    if (matches(first_sector, 'B')) return 'B';
    return '?';
}

bool pair_valid()
{
    const char primary = identify(0), secondary = identify(SLOT_SECTORS);
    return (primary == 'A' && secondary == 'B') || (primary == 'B' && secondary == 'A');
}

const char *mode_name()
{
    switch (engine.mode()) {
    case Engine::Mode::Idle: return "Idle";
    case Engine::Mode::Forward: return "Forward";
    case Engine::Mode::Ready: return "Ready";
    case Engine::Mode::Trial: return "Trial";
    case Engine::Mode::Reverse: return "Reverse";
    }
    return "Unknown";
}

int status(int, char **)
{
    const bool loaded = engine.load();
    const char primary = identify(0), secondary = identify(SLOT_SECTORS);
    ESP_LOGI(TAG, "state=%s primary=%c secondary=%c boot_ok=%d",
             loaded ? mode_name() : "UNPROVISIONED_OR_INVALID", primary, secondary, boot_ok);
    return 0;
}

int seed(int argc, char **argv)
{
    if (argc != 2 || std::strcmp(argv[1], "ERASE") != 0) {
        std::puts("Usage: seed ERASE (destroys ONLY dummy data and recovery evidence in swap_lab)");
        return 1;
    }
    boot_ok = false;
    // Invalidate both journal records BEFORE replacing any dummy image bytes.
    for (unsigned s = 7; s < Engine::sector_count; ++s) if (!flash.erase(s)) return 1;
    for (unsigned s = 0; s < 7; ++s) if (!flash.erase(s)) return 1;
    for (unsigned slot = 0; slot < 2; ++slot) {
        for (unsigned s = 0; s < SLOT_SECTORS; ++s) {
            for (unsigned i = 0; i < SECTOR; ++i)
                check_buffer[i] = pattern(slot == 0 ? 'A' : 'B', s * SECTOR + i);
            if (!flash.write(slot * SLOT_SECTORS + s, 0, check_buffer.data(), SECTOR)) return 1;
        }
    }
    if (!matches(0, 'A') || !matches(SLOT_SECTORS, 'B') || !engine.provision()) {
        ESP_LOGE(TAG, "Seed failed; do not request swap");
        return 1;
    }
    boot_ok = true;
    return status(0, nullptr);
}

int reset(int, char **)
{
    ESP_LOGW(TAG, "Reset: unconfirmed Trial data will revert on startup");
    std::fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_restart();
    return 0;
}

int swap(int, char **)
{
    if (!boot_ok || !engine.load() || engine.mode() != Engine::Mode::Idle || !pair_valid()) {
        ESP_LOGE(TAG, "Swap refused: need healthy Idle and complete A/B pair");
        return 1;
    }
    boot_ok = false;
    if (!engine.request_update()) return 1;
    return reset(0, nullptr);
}

int confirm(int, char **)
{
    if (!boot_ok || !engine.load() || engine.mode() != Engine::Mode::Trial || !pair_valid()) {
        ESP_LOGE(TAG, "Confirm refused: need healthy Trial and complete A/B pair");
        return 1;
    }
    boot_ok = false;
    if (!engine.confirm()) return 1;
    boot_ok = true;
    return status(0, nullptr);
}

void register_command(const char *name, const char *help, esp_console_cmd_func_t fn)
{
    esp_console_cmd_t cmd{};
    cmd.command = name;
    cmd.help = help;
    cmd.func = fn;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
} // namespace

extern "C" void app_main()
{
    uint32_t flash_size = 0;
    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
        static_cast<esp_partition_subtype_t>(0x40), "swap_lab");
    if (!partition || partition->address != LAB_ADDRESS ||
        partition->size != Engine::sector_count * SECTOR || partition->encrypted ||
        esp_flash_encryption_enabled() || esp_flash_get_size(nullptr, &flash_size) != ESP_OK ||
        flash_size < LAB_ADDRESS + Engine::sector_count * SECTOR) {
        ESP_LOGE(TAG, "STOP: need plaintext 4MB flash and exact swap_lab partition; nothing written");
        return;
    }
    ESP_LOGW(TAG, "DUMMY DATA LAB ONLY: no UWB, no image execution, no custom bootloader");
    ESP_LOGI(TAG, "swap_lab address=0x%lx size=0x%lx reset_reason=%d",
             static_cast<unsigned long>(partition->address),
             static_cast<unsigned long>(partition->size), static_cast<int>(esp_reset_reason()));
    // Called once per real reset. A second invocation would simulate another boot.
    boot_ok = engine.boot();
    if (boot_ok && !pair_valid()) {
        boot_ok = false;
        ESP_LOGE(TAG, "STOP: complete A/B pair check failed; preserve log before seed");
    }
    if (!boot_ok) ESP_LOGW(TAG, "Unprovisioned or recovery failed; inspect status, no auto erase");
    status(0, nullptr);

    esp_console_repl_t *repl = nullptr;
    esp_console_repl_config_t config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    config.prompt = "swap-lab>";
    config.task_stack_size = 6144;
    esp_console_dev_uart_config_t uart = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart, &config, &repl));
    ESP_ERROR_CHECK(esp_console_register_help_command());
    register_command("status", "Read journal state and compare all dummy A/B bytes", status);
    register_command("seed", "seed ERASE: destroy lab data, create Primary=A Secondary=B", seed);
    register_command("swap", "Persist swap request and reset (Idle only)", swap);
    register_command("confirm", "Accept swapped dummy data (Trial only)", confirm);
    register_command("reset", "Restart; unconfirmed Trial reverts", reset);
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
