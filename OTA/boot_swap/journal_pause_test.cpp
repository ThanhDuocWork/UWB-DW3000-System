#include "swap_core.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

constexpr unsigned SECTOR = 4096;
constexpr unsigned SLOT_SECTORS = 3;
constexpr unsigned SLOT = SECTOR * SLOT_SECTORS;
using Engine = boot_swap::Core<SECTOR, SLOT_SECTORS>;
struct Paused {};
static void check(bool ok)
{
    if (!ok) throw std::runtime_error("journal pause invariant failed");
}

struct Memory : Engine::Flash {
    std::array<uint8_t, Engine::sector_count * SECTOR> bytes{};
    bool armed = false;
    unsigned paused_sector = 0;
    unsigned paused_offset = 0;
    unsigned sealed_sector = 0;
    unsigned sealed_offset = 0;
    unsigned journal_erases = 0;
    bool pause_on_rotation = false;
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
        if (s == 7 || s == 8) ++journal_erases;
        std::fill_n(bytes.begin() + s * SECTOR, SECTOR, uint8_t{0xff});
        return true;
    }
    bool write(unsigned s, unsigned o, const void *p, unsigned n) override
    {
        check(s < Engine::sector_count && o <= SECTOR && n <= SECTOR - o);
        const auto *src = static_cast<const uint8_t *>(p);
        for (unsigned i = 0; i < n; ++i) {
            auto &dest = bytes[s * SECTOR + o + i];
            check((dest & src[i]) == src[i]);
            dest &= src[i];
        }
        // Same completed-body boundary as LabFlash; unwind instead of hanging.
        if (armed && (s == 7 || s == 8) && o % 32 == 0 && n == 28 &&
            (!pause_on_rotation || o == 0)) {
            armed = false;
            paused_sector = s;
            paused_offset = o;
            check(std::equal(src, src + n, bytes.begin() + s * SECTOR + o));
            throw Paused{};
        }
        if ((s == 7 || s == 8) && o % 32 == 28 && n == 4) {
            sealed_sector = s;
            sealed_offset = o;
        }
        return true;
    }
};

static unsigned run(bool checkpoint, bool reverse_pair, bool alternate_parity)
{
    Memory m;
    for (unsigned i = 0; i < SLOT; ++i) {
        const auto a = static_cast<uint8_t>(i * 17U + 3U);
        const auto b = static_cast<uint8_t>(i * 31U + 7U);
        m.bytes[i] = reverse_pair ? b : a;
        m.bytes[SLOT + i] = reverse_pair ? a : b;
    }
    Engine engine(m);
    check(engine.provision());
    const auto original = m.bytes;
    if (alternate_parity) {
        // A full unconfirmed swap/rollback adds 21 commits, flipping Idle parity.
        check(engine.request_update());
        Engine trial(m);
        check(trial.boot() && trial.mode() == Engine::Mode::Trial);
        Engine rollback(m);
        check(rollback.boot() && rollback.mode() == Engine::Mode::Idle);
    }
    check(engine.load() && engine.mode() == Engine::Mode::Idle);
    check(std::equal(m.bytes.begin(), m.bytes.begin() + 2 * SLOT, original.begin()));
    if (checkpoint) check(engine.request_update());
    const auto prior = m.bytes;
    const unsigned prior_sector = m.sealed_sector;
    const unsigned prior_offset = m.sealed_offset;
    m.armed = true;
    bool paused = false;
    try {
        if (checkpoint) (void)engine.boot();
        else (void)engine.request_update();
    } catch (const Paused &) { paused = true; }
    check(paused && !m.armed);
    check(m.paused_sector == prior_sector);
    check(m.paused_offset == prior_offset + 4);
    check(std::equal(m.bytes.begin() + prior_sector * SECTOR + prior_offset - 28,
                     m.bytes.begin() + prior_sector * SECTOR + prior_offset + 4,
                     prior.begin() + prior_sector * SECTOR + prior_offset - 28));
    check(std::all_of(m.bytes.begin() + m.paused_sector * SECTOR + m.paused_offset + 28,
                      m.bytes.begin() + m.paused_sector * SECTOR + m.paused_offset + 32,
                      [](uint8_t b) { return b == 0xff; }));
    check(std::equal(m.bytes.begin(), m.bytes.begin() + 2 * SLOT, original.begin()));
    Engine persisted(m);
    check(persisted.load());
    check(persisted.mode() == (checkpoint ? Engine::Mode::Forward : Engine::Mode::Idle));

    // Recreate all adapter and engine RAM, carrying only persisted bytes.
    Memory reset;
    reset.bytes = m.bytes;
    Engine reboot(reset);
    check(reboot.boot());
    check(reboot.mode() == (checkpoint ? Engine::Mode::Trial : Engine::Mode::Idle));
    check(std::equal(reset.bytes.begin(), reset.bytes.begin() + SLOT,
                     original.begin() + (checkpoint ? SLOT : 0)));
    check(std::equal(reset.bytes.begin() + SLOT, reset.bytes.begin() + 2 * SLOT,
                     original.begin() + (checkpoint ? 0 : SLOT)));
    Memory next_reset;
    next_reset.bytes = reset.bytes;
    Engine next(next_reset);
    check(next.boot() && next.mode() == Engine::Mode::Idle);
    check(std::equal(next_reset.bytes.begin(), next_reset.bytes.begin() + 2 * SLOT,
                     original.begin()));
    return m.paused_offset;
}

static void run_rotation()
{
    Memory m;
    Engine engine(m);
    check(engine.provision());
    for (unsigned update = 0; update < 10; ++update) {
        check(engine.request_update());
        check(engine.boot() && engine.mode() == Engine::Mode::Trial);
        check(engine.confirm());
    }
    check(m.journal_erases == 0);
    check(engine.request_update());
    m.armed = true;
    m.pause_on_rotation = true;
    bool paused = false;
    try { (void)engine.boot(); } catch (const Paused &) { paused = true; }
    check(paused && m.paused_sector == 8 && m.paused_offset == 0);
    check(m.journal_erases == 1);
    Engine recovered(m);
    check(recovered.load() && recovered.mode() == Engine::Mode::Forward);
    check(recovered.boot() && recovered.mode() == Engine::Mode::Trial);
    check(recovered.confirm());
    Engine persisted(m);
    check(persisted.load() && persisted.mode() == Engine::Mode::Idle);
    check(m.journal_erases == 2);
}

int main()
{
    for (bool checkpoint : {false, true}) {
        for (bool reverse_pair : {false, true}) {
            const unsigned first = run(checkpoint, reverse_pair, false);
            const unsigned second = run(checkpoint, reverse_pair, true);
            check(first != second);
        }
    }
    run_rotation();
    std::cout << "TC09: 8 journal pause cases; append offsets/orientations, prior record, "
                 "erased seal, request Idle, checkpoint Trial and rollback passed\n";
}
