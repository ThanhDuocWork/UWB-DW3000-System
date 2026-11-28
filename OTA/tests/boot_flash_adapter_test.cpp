#include "boot_flash_adapter.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <algorithm>

using namespace boot_swap;

static void check(bool value)
{
    if (!value) throw std::runtime_error("boot flash adapter invariant failed");
}

struct Fake {
    static inline std::array<uint8_t, kFlashSize> bytes{};
    static inline uint32_t last_address = 0;
    static inline uint32_t last_erased = 0;
    static inline unsigned operations = 0;
    static bool read(uint32_t address, void *data, unsigned size)
    {
        check(address % 4 == 0 && size % 4 == 0 && address + size <= kFlashSize);
        std::memcpy(data, bytes.data() + address, size);
        last_address = address;
        ++operations;
        return true;
    }
    static bool write(uint32_t address, const void *data, unsigned size)
    {
        check(address % 4 == 0 && size % 4 == 0 && address + size <= kFlashSize);
        std::memcpy(bytes.data() + address, data, size);
        last_address = address;
        ++operations;
        return true;
    }
    static bool erase(uint32_t sector)
    {
        check(sector < kFlashSize / kSectorSize);
        last_erased = sector;
        ++operations;
        return true;
    }
};

int main()
{
    constexpr unsigned count = sizeof(kLayout) / sizeof(kLayout[0]);
    const FlashOps ops{Fake::read, Fake::write, Fake::erase};
    BootFlashAdapter flash(kLayout, count, kFlashSize, false, ops);
    check(flash.ready());

    struct Point { unsigned sector; uint32_t address; };
    constexpr Point points[] = {
        {0, 0x20000}, {kSlotSectors - 1, 0x19f000},
        {kSlotSectors, 0x1e0000}, {2 * kSlotSectors - 1, 0x35f000},
        {2 * kSlotSectors, 0x360000},
        {2 * kSlotSectors + 1, 0x361000},
        {2 * kSlotSectors + 2, 0x362000},
    };
    std::array<uint8_t, kSectorSize> source{}, result{};
    source.fill(0xa5);
    for (const auto &point : points) {
        check(flash.erase(point.sector) &&
              Fake::last_erased == point.address / kSectorSize);
        check(flash.write(point.sector, 0, source.data(), kSectorSize));
        check(Fake::last_address == point.address + kSectorSize - 256);
        check(flash.read(point.sector, 0, result.data(), kSectorSize));
        check(result == source);
    }
    check(flash.write(2 * kSlotSectors + 1, 28, source.data(), 4));
    check(Fake::last_address == 0x361000 + 28);
    const unsigned before = Fake::operations;
    check(!flash.read(BootCore::sector_count, 0, result.data(), 4));
    check(!flash.write(0, kSectorSize - 2, source.data(), 4));
    check(!flash.erase(BootCore::sector_count));
    check(!flash.read(0, 2, result.data(), 4));
    check(Fake::operations == before);

    auto wrong = std::array<Partition, count>{};
    for (unsigned i = 0; i < count; ++i) wrong[i] = kLayout[i];
    wrong[4].offset += kSectorSize;
    check(!BootFlashAdapter(wrong.data(), count, kFlashSize, false, ops).ready());
    wrong[4] = kLayout[4];
    wrong[7].label = "journal_a";
    check(!BootFlashAdapter(wrong.data(), count, kFlashSize, false, ops).ready());
    check(!BootFlashAdapter(kLayout, count, kFlashSize, true, ops).ready());
    check(!BootFlashAdapter(kLayout, count, kFlashSize / 2, false, ops).ready());
    check(!BootFlashAdapter(kLayout, count, kFlashSize, false, {}).ready());
    check(!BootFlashAdapter(kLayout, count - 1, kFlashSize, false, ops).ready());
    Fake::bytes.fill(0xff);
    BootCore::Mode mode{};
    uint32_t sequence = 0;
    check(!BootCore::inspect_journal(flash, mode, sequence));
    BootCore core(flash);
    check(core.provision());
    check(BootCore::inspect_journal(flash, mode, sequence));
    check(mode == BootCore::Mode::Idle && sequence == 1);
    check(!core.provision());
    Fake::bytes[kLayout[6].offset] ^= 1;
    check(!BootCore::inspect_journal(flash, mode, sequence));
    std::cout << "boot adapter layout, mapping, alignment and rejection tests passed\n";
}
