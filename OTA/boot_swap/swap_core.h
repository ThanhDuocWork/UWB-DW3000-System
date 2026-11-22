#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

namespace boot_swap {

// Logical sectors shared by the host tests and physical flash adapters.
// Primary [0,N), secondary [N,2N), scratch 2N, journal 2N+1 and 2N+2.
template <unsigned SectorSize, unsigned SlotSectors> class Core {
public:
    static_assert(SectorSize >= 32 && SectorSize % 4 == 0 && SlotSectors > 0);
    static_assert(SectorSize % 32 == 0 && SectorSize / 32 >= 2);
    static constexpr unsigned sector_count = 2 * SlotSectors + 3;
    struct Flash {
        virtual ~Flash() = default;
        virtual bool read(unsigned sector, unsigned offset, void *data, unsigned size) = 0;
        virtual bool erase(unsigned sector) = 0;
        virtual bool write(unsigned sector, unsigned offset, const void *data, unsigned size) = 0;
    };
    enum class Mode : uint32_t { Idle, Forward, Ready, Trial, Reverse };
    enum class JournalStatus { Valid, Blank, Corrupt, IoError };
    explicit Core(Flash &flash) : flash_(flash) {}

    // Explicit provisioning only; never silently format a damaged journal.
    bool provision()
    {
        for (unsigned j = 0; j < 2; ++j) {
            if (!flash_.read(journal + j, 0, buffer_.data(), SectorSize)) return false;
            for (auto b : buffer_) if (b != 0xff) return false;
        }
        active_ = 0;
        next_entry_ = 0;
        state_ = {magic, SlotSectors, 0, 0, 0, 0, 0, seal};
        loaded_ = true;
        return commit(Mode::Idle, 0, 0);
    }

    bool load()
    {
        return load_status() == JournalStatus::Valid;
    }

    JournalStatus load_status()
    {
        loaded_ = false;
        const auto status = scan_journal(flash_, state_, active_, next_entry_);
        loaded_ = status == JournalStatus::Valid;
        return status;
    }

    Mode mode() const { return static_cast<Mode>(state_[3]); }

    // Read-only journal inspection for the bootloader diagnostic hook.
    static bool inspect_journal(Flash &flash, Mode &mode, uint32_t &sequence)
    {
        return inspect_status(flash, mode, sequence) == JournalStatus::Valid;
    }

    static JournalStatus inspect_status(Flash &flash, Mode &mode, uint32_t &sequence)
    {
        Record latest{};
        unsigned active = 0, next_entry = 0;
        const auto status = scan_journal(flash, latest, active, next_entry);
        if (status != JournalStatus::Valid) return status;
        mode = static_cast<Mode>(latest[3]);
        sequence = latest[2];
        return status;
    }

    // Caller must validate both images, target/role, and size BEFORE requesting.
    // No staging writes are permitted after this request until confirm/revert.
    bool request_update()
    {
        return loaded_ && mode() == Mode::Idle && commit(Mode::Forward, 0, 0);
    }

    // Called once per reset by the boot policy, before executing Primary.
    // false means STOP: do not jump to an image that may be partially swapped.
    bool boot()
    {
        if (!load()) return false;
        if (mode() == Mode::Trial && !commit(Mode::Reverse, 0, 0)) return false;
        while (mode() == Mode::Forward || mode() == Mode::Reverse) {
            if (!step()) return false;
        }
        if (mode() == Mode::Ready) return commit(Mode::Trial, 0, 0);
        return mode() == Mode::Idle;
    }

    bool reject_ready()
    {
        return loaded_ && mode() == Mode::Ready && commit(Mode::Reverse, 0, 0);
    }

    bool confirm()
    {
        return loaded_ && mode() == Mode::Trial && commit(Mode::Idle, 0, 0);
    }

private:
    using Record = std::array<uint32_t, 8>;
    static constexpr uint32_t magic = 0x53575031; // Format v1, little-endian targets.
    static constexpr uint32_t seal = 0x434f4d54;
    static constexpr unsigned entries_per_journal = SectorSize / sizeof(Record);
    static constexpr unsigned scratch = 2 * SlotSectors;
    static constexpr unsigned journal = scratch + 1;
    Flash &flash_;
    Record state_{};
    unsigned active_ = 0;
    unsigned next_entry_ = 0;
    bool loaded_ = false;
    std::array<uint8_t, SectorSize> buffer_{}, verify_{};

    static uint32_t crc(const Record &r)
    {
        uint32_t value = 0xffffffff;
        for (unsigned i = 0; i < 6; ++i) {
            for (unsigned byte = 0; byte < 4; ++byte) {
                value ^= (r[i] >> (byte * 8)) & 0xff;
                for (unsigned bit = 0; bit < 8; ++bit)
                    value = (value >> 1) ^ ((value & 1) ? 0xedb88320 : 0);
            }
        }
        return ~value;
    }

    static bool valid(const Record &r)
    {
        if (r[0] != magic || r[1] != SlotSectors || r[2] == 0 ||
            r[3] > static_cast<uint32_t>(Mode::Reverse) ||
            r[6] != crc(r) || r[7] != seal) return false;
        const auto m = static_cast<Mode>(r[3]);
        if (m == Mode::Forward || m == Mode::Reverse)
            return r[4] < SlotSectors && r[5] < 3;
        return r[4] == 0 && r[5] == 0;
    }

    static JournalStatus scan_journal(Flash &flash, Record &latest, unsigned &active,
                             unsigned &next_entry)
    {
        bool found = false;
        bool blank = true, ambiguous = false, io_error = false;
        unsigned last_used[2] = {0, 0};
        for (unsigned j = 0; j < 2; ++j) {
            for (unsigned entry = 0; entry < entries_per_journal; ++entry) {
                Record candidate{};
                if (!flash.read(journal + j, entry * sizeof(Record),
                                candidate.data(), sizeof(candidate))) {
                    io_error = true;
                    continue;
                }
                bool empty = true;
                for (uint32_t word : candidate) empty = empty && word == 0xffffffffU;
                if (!empty) last_used[j] = entry + 1;
                blank = blank && empty;
                if (!valid(candidate)) continue;
                if (found && candidate[2] == latest[2]) ambiguous = true;
                if (!found || candidate[2] > latest[2]) {
                    latest = candidate;
                    active = j;
                    found = true;
                }
            }
        }
        if (found) next_entry = last_used[active];
        if (io_error) return JournalStatus::IoError;
        if (ambiguous) return JournalStatus::Corrupt;
        if (found) return JournalStatus::Valid;
        return blank ? JournalStatus::Blank : JournalStatus::Corrupt;
    }

    bool commit(Mode m, unsigned sector, unsigned phase)
    {
        if (state_[2] == std::numeric_limits<uint32_t>::max()) return false;
        Record next{magic, SlotSectors, state_[2] + 1,
                    static_cast<uint32_t>(m), sector, phase, 0, seal};
        next[6] = crc(next);
        const bool rotate = next_entry_ == entries_per_journal;
        const unsigned dest = rotate ? active_ ^ 1 : active_;
        const unsigned entry = rotate ? 0 : next_entry_;
        const unsigned offset = entry * sizeof(Record);
        // Keep the previous record intact until the new record is committed.
        loaded_ = false;
        if ((rotate && !flash_.erase(journal + dest)) ||
            !flash_.write(journal + dest, offset, next.data(), 28) ||
            !flash_.write(journal + dest, offset + 28, &next[7], 4)) return false;
        Record check{};
        if (!flash_.read(journal + dest, offset, check.data(), sizeof(check)) || check != next)
            return false;
        state_ = next;
        active_ = dest;
        next_entry_ = entry + 1;
        loaded_ = true;
        return true;
    }

    bool copy(unsigned source, unsigned dest)
    {
        // Source remains intact across erase/program failure: repeating is safe.
        return flash_.read(source, 0, buffer_.data(), SectorSize) &&
               flash_.erase(dest) &&
               flash_.write(dest, 0, buffer_.data(), SectorSize) &&
               flash_.read(dest, 0, verify_.data(), SectorSize) && buffer_ == verify_;
    }

    bool step()
    {
        const auto m = mode();
        const unsigned i = state_[4], phase = state_[5];
        if (phase == 0) {
            if (!copy(SlotSectors + i, scratch)) return false;
            return commit(m, i, 1);
        }
        if (phase == 1) {
            if (!copy(i, SlotSectors + i)) return false;
            return commit(m, i, 2);
        }
        if (!copy(scratch, i)) return false;
        if (i + 1 < SlotSectors) return commit(m, i + 1, 0);
        return commit(m == Mode::Forward ? Mode::Ready : Mode::Idle, 0, 0);
    }
};

} // namespace boot_swap
