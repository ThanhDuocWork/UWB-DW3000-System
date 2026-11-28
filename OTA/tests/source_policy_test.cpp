#include "boot_flash_adapter.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

static void check(bool ok) {
    if (!ok) throw std::runtime_error("source policy invariant failed");
}
static std::string read(const std::string &path) {
    std::ifstream in(path); check(in.good());
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
static void has(const std::string &s, const std::string &text) {
    if (s.find(text) == std::string::npos) throw std::runtime_error("missing policy: " + text);
}
static void ordered(const std::string &s, const std::string &before, const std::string &after) {
    has(s, before); has(s, after); check(s.find(before) < s.find(after));
}
static unsigned count(const std::string &s, const std::string &text) {
    unsigned n = 0;
    for (size_t pos = 0; (pos = s.find(text, pos)) != std::string::npos; pos += text.size()) ++n;
    return n;
}
int main(int argc, char **argv) {
    check(argc == 2); const std::string root = argv[1];
    const auto hook = read(root + "/firmware_update/bootloader_components/boot_swap_hook/boot_swap_hook.cpp");
    has(hook, "#include \"bootloader_utility.h\"");
    has(hook, "if (!inspect()) {"); has(hook, "bootloader_reset();");
    has(hook, "alignas(boot_swap::BootCore) unsigned char core_storage");
    has(hook, "alignas(boot_swap::BootFlashAdapter) unsigned char flash_storage");
    check(count(hook, "new (core_storage) boot_swap::BootCore") == 1);
    check(hook.find("boot_swap::BootCore core(") == std::string::npos);
    ordered(hook, "if (!layout_ok)", "new (core_storage)");
    ordered(hook, "if (status == Status::Corrupt || status == Status::IoError) return false;",
            "boot_swap::boot_flow");
    has(hook, "journal status=%s mode=%lu seq=%lu");
    has(hook, "commissioning A.1; provision required");
    has(hook, "boot policy PASS: %s");
    check(hook.find("verify_image(boot_swap::kLayout[4]") == std::string::npos);
    const auto app = read(root + "/firmware_update/main/firmware_update.cpp");
    check(count(app, "static boot_swap::BootCore core(flash)") == 1);
    check(count(app, "static boot_swap::BootFlashAdapter flash(") == 1);
    has(app, "esp_image_verify(ESP_IMAGE_VERIFY_SILENT");
    has(app, "repl_config.task_stack_size = 16384;");
    has(app, "log_slot(\"Primary\""); has(app, "log_slot(\"Secondary\"");
    check(app.find("DISABLED") == std::string::npos);
    check(app.find("read-only hook") == std::string::npos);
    const auto swap = app.substr(app.find("int swap_command"),
                                 app.find("int confirm_command") - app.find("int swap_command"));
    ordered(swap, "actual_layout_valid()", "core.request_update()");
    ordered(swap, "running_is(\"A.1\")", "core.request_update()");
    ordered(swap, "valid_image(PRIMARY_ADDRESS, \"A.1\")", "core.request_update()");
    ordered(swap, "valid_image(SECONDARY_ADDRESS, \"B.1\")", "core.request_update()");
    const auto confirm = app.substr(app.find("int confirm_command"),
                                    app.find("void log_slot") - app.find("int confirm_command"));
    ordered(confirm, "actual_layout_valid()", "core.confirm()");
    ordered(confirm, "running_is(\"B.1\")", "core.confirm()");
    ordered(confirm, "valid_image(PRIMARY_ADDRESS, \"B.1\")", "core.confirm()");
    ordered(confirm, "const auto mode = core.mode()", "if (!loaded || mode != boot_swap::BootCore::Mode::Trial)");
    ordered(confirm, "if (!loaded || mode != boot_swap::BootCore::Mode::Trial)", "core.confirm()");
    const auto provision = app.substr(app.find("int provision_command"));
    ordered(provision, "actual_layout_valid()", "core.provision()");
    ordered(provision, "valid_image(PRIMARY_ADDRESS, \"A.1\")", "core.provision()");
    ordered(provision, "valid_image(SECONDARY_ADDRESS, \"B.1\")", "core.provision()");
    const auto ops = read(root + "/boot_swap/esp32_boot_flash.cpp");
    has(ops, "#include \"hal/wdt_hal.h\"");
    has(ops, "wdt_hal_context_t rtc_wdt_ctx = RWDT_HAL_CONTEXT_DEFAULT();");
    ordered(ops, "wdt_hal_write_protect_disable(&rtc_wdt_ctx);",
            "wdt_hal_feed(&rtc_wdt_ctx);");
    ordered(ops, "wdt_hal_feed(&rtc_wdt_ctx);",
            "wdt_hal_write_protect_enable(&rtc_wdt_ctx);");
    check(count(ops, "wdt_hal_feed(&rtc_wdt_ctx);") == 1);
    check(count(ops, "feed_watchdog();") == 3);
    check(count(ops, "return false;") == 3);
    for (const auto *operation : {"bool flash_read(", "bool flash_write(", "bool flash_erase("}) {
        const auto begin = ops.find(operation); check(begin != std::string::npos);
        const auto body = ops.substr(begin, ops.find('}', begin) - begin);
        ordered(body, "return false;", "feed_watchdog();");
        ordered(body, "feed_watchdog();", "return true;");
    }
    check(ops.find("rtc_wdt.h") == std::string::npos);
    check(ops.find("rtc_wdt_feed") == std::string::npos);
    check(ops.find("rtc_wdt_disable") == std::string::npos);
    check(ops.find("wdt_hal_disable") == std::string::npos);
    check(ops.find("wdt_hal_init") == std::string::npos);
    const auto component = read(root + "/firmware_update/bootloader_components/boot_swap_hook/CMakeLists.txt");
    has(component, "PRIV_REQUIRES bootloader_support esp_app_format spi_flash hal)");
    static_assert(sizeof(boot_swap::BootCore) + sizeof(boot_swap::BootFlashAdapter) < 0x2800,
                  "Keep core/adapter storage below 10 KiB; total target DRAM needs linker verification");
    std::cout << "hook/app source-policy checks passed; host core+adapter bytes="
              << sizeof(boot_swap::BootCore) + sizeof(boot_swap::BootFlashAdapter) << '\n';
}
