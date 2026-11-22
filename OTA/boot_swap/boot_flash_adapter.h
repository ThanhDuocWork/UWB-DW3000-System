#pragma once

#include "swap_core.h"

#include <cstdint>
#include <cstring>

namespace boot_swap {

constexpr unsigned kSectorSize = 0x1000;
constexpr unsigned kSlotSectors = 0x180000 / kSectorSize;
constexpr unsigned kFlashSize = 0x400000;
using BootCore = Core<kSectorSize, kSlotSectors>;

struct Partition {
    const char *label;
    uint8_t type;
    uint8_t subtype;
    uint32_t offset;
    uint32_t size;
    uint32_t flags;
};

constexpr Partition kLayout[] = {
    {"nvs",       1, 0x02, 0x011000, 0x006000, 0},
    {"phy_init",  1, 0x01, 0x017000, 0x001000, 0},
    {"factory",   0, 0x00, 0x020000, 0x180000, 0},
    {"storage",   1, 0x82, 0x1a0000, 0x040000, 0},
    {"secondary", 1, 0x40, 0x1e0000, 0x180000, 0},
    {"scratch",   1, 0x41, 0x360000, 0x001000, 0},
    {"journal_a", 1, 0x42, 0x361000, 0x001000, 0},
    {"journal_b", 1, 0x43, 0x362000, 0x001000, 0},
};
static_assert(kLayout[2].size == kSlotSectors * kSectorSize);
static_assert(kLayout[4].size == kLayout[2].size);
static_assert(kLayout[7].offset + kLayout[7].size <= kFlashSize);

inline bool layout_valid(const Partition *parts, unsigned count,
                         uint32_t flash_size, bool encrypted)
{
    if (!parts || encrypted || flash_size != kFlashSize ||
        count != sizeof(kLayout) / sizeof(kLayout[0])) return false;
    for (unsigned i = 0; i < count; ++i) {
        const auto &expected = kLayout[i];
        unsigned matches = 0;
        for (unsigned j = 0; j < count; ++j) {
            const auto &actual = parts[j];
            if (!actual.label) return false;
            if (std::strcmp(actual.label, expected.label) != 0) continue;
            if (actual.type != expected.type || actual.subtype != expected.subtype ||
                actual.offset != expected.offset || actual.size != expected.size ||
                actual.flags != expected.flags ||
                actual.offset % kSectorSize != 0 ||
                actual.size % kSectorSize != 0 ||
                actual.offset > flash_size ||
                actual.size > flash_size - actual.offset) return false;
            ++matches;
        }
        if (matches != 1) return false;
    }
    return true;
}

// All callbacks use physical flash addresses; pointers and lengths are 4-byte aligned.
struct FlashOps {
    bool (*read)(uint32_t address, void *data, unsigned size);
    bool (*write)(uint32_t address, const void *data, unsigned size);
    bool (*erase_sector)(uint32_t absolute_sector);
};

// Implemented only for the ESP-IDF second-stage bootloader target.
FlashOps esp32_boot_flash_ops();

class BootFlashAdapter final : public BootCore::Flash {
public:
    BootFlashAdapter(const Partition *parts, unsigned count, uint32_t flash_size,
                     bool encrypted, FlashOps ops)
        : ready_(layout_valid(parts, count, flash_size, encrypted) &&
                 ops.read && ops.write && ops.erase_sector), ops_(ops) {}

    bool ready() const { return ready_; }

    bool read(unsigned sector, unsigned offset, void *data, unsigned size) override
    {
        uint32_t address;
        if (!data || !resolve(sector, offset, size, address) ||
            (size % 4) != 0 || (offset % 4) != 0) return false;
        auto *bytes = static_cast<uint8_t *>(data);
        for (unsigned done = 0; done < size; done += kChunk) {
            const unsigned n = size - done < kChunk ? size - done : kChunk;
            if (!ops_.read(address + done, staging_, n)) return false;
            std::memcpy(bytes + done, staging_, n);
        }
        return true;
    }

    bool write(unsigned sector, unsigned offset, const void *data, unsigned size) override
    {
        uint32_t address;
        if (!data || !resolve(sector, offset, size, address) ||
            (size % 4) != 0 || (offset % 4) != 0) return false;
        const auto *bytes = static_cast<const uint8_t *>(data);
        for (unsigned done = 0; done < size; done += kChunk) {
            const unsigned n = size - done < kChunk ? size - done : kChunk;
            std::memcpy(staging_, bytes + done, n);
            if (!ops_.write(address + done, staging_, n)) return false;
        }
        return true;
    }

    bool erase(unsigned sector) override
    {
        uint32_t address;
        return resolve(sector, 0, kSectorSize, address) &&
               ops_.erase_sector(address / kSectorSize);
    }

private:
    static constexpr unsigned kChunk = 256;
    bool ready_;
    FlashOps ops_;
    alignas(4) uint8_t staging_[kChunk]{};

    bool resolve(unsigned sector, unsigned offset, unsigned size,
                 uint32_t &address) const
    {
        if (!ready_ || sector >= BootCore::sector_count ||
            offset > kSectorSize || size > kSectorSize - offset) return false;
        if (sector < kSlotSectors) {
            address = kLayout[2].offset + sector * kSectorSize + offset;
        } else if (sector < 2 * kSlotSectors) {
            address = kLayout[4].offset +
                      (sector - kSlotSectors) * kSectorSize + offset;
        } else {
            address = kLayout[5 + sector - 2 * kSlotSectors].offset + offset;
        }
        return true;
    }
};

}  // namespace boot_swap
