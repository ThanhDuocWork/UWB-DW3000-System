#include "swap_core.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

constexpr unsigned SECTOR = 4096;
constexpr unsigned SLOT_SECTORS = 3;
using Engine = boot_swap::Core<SECTOR, SLOT_SECTORS>;
struct Paused {};
static void check(bool ok)
{
    if (!ok) throw std::runtime_error("scratch pause invariant failed");
}

struct Memory : Engine::Flash {
    std::array<uint8_t, Engine::sector_count * SECTOR> bytes{};
    bool armed = false;
    Memory() { bytes.fill(0xff); }
    bool read(unsigned s, unsigned o, void *p, unsigned n) override
    {
        check(s < Engine::sector_count && o <= SECTOR && n <= SECTOR - o);
        std::memcpy(p, bytes.data() + s * SECTOR + o, n);
        return true;
    }
    bool erase(unsigned s) override
    {
        check(s < Engine::sector_count);
        std::fill_n(bytes.begin() + s * SECTOR, SECTOR, uint8_t{0xff});
        return true;
    }
    bool write(unsigned s, unsigned o, const void *p, unsigned n) override
    {
        check(s < Engine::sector_count && o <= SECTOR && n <= SECTOR - o);
        const auto *src = static_cast<const uint8_t *>(p);
        for (unsigned done = 0; done < n;) {
            const unsigned chunk = std::min(256U, n - done);
            for (unsigned i = 0; i < chunk; ++i) {
                auto &dest = bytes[s * SECTOR + o + done + i];
                check((dest & src[done + i]) == src[done + i]);
                dest &= src[done + i];
            }
            // Same post-success checkpoint as LabFlash; unwind instead of hanging.
            if (armed && s == 2 * SLOT_SECTORS && o + done == 0 && chunk == 256) {
                armed = false;
                throw Paused{};
            }
            done += chunk;
        }
        return true;
    }
};

int main()
{
    constexpr unsigned slot = SLOT_SECTORS * SECTOR;
    constexpr unsigned scratch = 2 * slot;
    constexpr unsigned journal = scratch + SECTOR;
    Memory m;
    for (unsigned i = 0; i < slot; ++i) {
        m.bytes[i] = static_cast<uint8_t>(i * 17U + 3U);
        m.bytes[slot + i] = static_cast<uint8_t>(i * 31U + 7U);
    }
    Engine engine(m);
    check(engine.provision());
    const auto original = m.bytes;
    check(engine.request_update());
    const auto requested = m.bytes;
    m.armed = true;
    bool paused = false;
    try { (void)engine.boot(); } catch (const Paused &) { paused = true; }
    check(paused && !m.armed);
    check(std::equal(m.bytes.begin(), m.bytes.begin() + scratch, original.begin()));
    check(std::equal(m.bytes.begin() + scratch, m.bytes.begin() + scratch + 256,
                     original.begin() + slot));
    check(std::all_of(m.bytes.begin() + scratch + 256, m.bytes.begin() + journal,
                      [](uint8_t b) { return b == 0xff; }));
    check(std::equal(m.bytes.begin() + journal, m.bytes.end(), requested.begin() + journal));

    // Recreate RAM state from only the persisted bytes, with the hook off.
    for (bool confirm : {false, true}) {
        Memory reset;
        reset.bytes = m.bytes;
        Engine reboot(reset);
        check(reboot.boot() && reboot.mode() == Engine::Mode::Trial);
        check(std::equal(reset.bytes.begin(), reset.bytes.begin() + slot, original.begin() + slot));
        check(std::equal(reset.bytes.begin() + slot, reset.bytes.begin() + scratch, original.begin()));
        if (confirm) check(reboot.confirm());
        Engine next(reset);
        check(next.boot() && next.mode() == Engine::Mode::Idle);
        check(std::equal(reset.bytes.begin(), reset.bytes.begin() + slot,
                         original.begin() + (confirm ? slot : 0)));
        check(std::equal(reset.bytes.begin() + slot, reset.bytes.begin() + scratch,
                         original.begin() + (confirm ? 0 : slot)));
    }
    std::cout << "scratch pause 256/4096: journal, sources, tail, Trial, rollback and confirm passed\n";
}
