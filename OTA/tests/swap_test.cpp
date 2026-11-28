#include "swap_core.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

using Engine = boot_swap::Core<64, 3>;
struct PowerLoss {};
static void check(bool ok)
{
    if (!ok) throw std::runtime_error("boot swap invariant failed");
}

struct Memory : Engine::Flash {
    std::array<uint8_t, Engine::sector_count * 64> bytes{};
    int cut = -1;
    int mutations = 0;
    Memory() { bytes.fill(0xff); }
    void touch()
    {
        if (mutations == cut) throw PowerLoss{};
        ++mutations;
    }
    bool read(unsigned s, unsigned o, void *p, unsigned n) override
    {
        check(s < Engine::sector_count && o + n <= 64);
        std::memcpy(p, bytes.data() + s * 64 + o, n);
        return true;
    }
    bool erase(unsigned s) override
    {
        check(s < Engine::sector_count);
        for (unsigned i = 0; i < 64; ++i) {
            touch();
            bytes[s * 64 + i] = 0xff;
        }
        return true;
    }
    bool write(unsigned s, unsigned o, const void *p, unsigned n) override
    {
        check(s < Engine::sector_count && o + n <= 64);
        const auto *src = static_cast<const uint8_t *>(p);
        for (unsigned i = 0; i < n; ++i) {
            touch();
            auto &dest = bytes[s * 64 + o + i];
            check((dest & src[i]) == src[i]); // NOR cannot program zero back to one.
            dest &= src[i];
        }
        return true;
    }
    void inject(int point) { cut = point; mutations = 0; }
};

static Memory initial()
{
    Memory m;
    for (unsigned i = 0; i < 192; ++i) {
        m.bytes[i] = static_cast<uint8_t>(i * 17 + 3);
        m.bytes[192 + i] = static_cast<uint8_t>(i * 31 + 7);
    }
    Engine e(m);
    check(e.provision());
    m.inject(-1);
    return m;
}

static bool swapped(const Memory &m, const Memory &original)
{
    return std::equal(m.bytes.begin(), m.bytes.begin() + 192, original.bytes.begin() + 192) &&
           std::equal(m.bytes.begin() + 192, m.bytes.begin() + 384, original.bytes.begin());
}

static bool unchanged(const Memory &m, const Memory &original)
{
    return std::equal(m.bytes.begin(), m.bytes.begin() + 384, original.bytes.begin());
}

// Crash before every individual emulated flash-byte mutation, including torn
// metadata and partial erase. Recreate the engine to discard all RAM state.
template <class Action, class Verify>
static void cuts(const Memory &start, Action action, Verify verify)
{
    auto baseline = start;
    baseline.inject(-1);
    action(baseline);
    const int total = baseline.mutations;
    for (int point = 0; point <= total; ++point) {
        auto m = start;
        m.inject(point);
        try { action(m); } catch (const PowerLoss &) {}
        m.inject(-1);
        Engine reboot(m);
        check(reboot.boot());
        verify(m, reboot);
    }
    std::cout << "checked " << total + 1 << " power-cut positions\n";
}

int main()
{
    const auto original = initial();
    Memory blank;
    Engine unprovisioned(blank);
    check(!unprovisioned.boot());

    cuts(original, [](Memory &m) {
        Engine e(m); check(e.load()); check(e.request_update());
    }, [&](Memory &m, Engine &e) {
        check(e.mode() == Engine::Mode::Trial ? swapped(m, original) : unchanged(m, original));
    });

    auto pending = original;
    Engine request(pending);
    check(request.load()); check(request.request_update());
    cuts(pending, [](Memory &m) {
        Engine e(m); check(e.boot());
    }, [&](Memory &m, Engine &e) {
        if (e.mode() == Engine::Mode::Trial) {
            check(swapped(m, original)); check(e.confirm());
            Engine next(m); check(next.boot()); check(swapped(m, original));
        } else {
            // Trial marker may be durable even if power failed before app boot.
            check(e.mode() == Engine::Mode::Idle && unchanged(m, original));
        }
    });

    auto trial = pending;
    Engine first_boot(trial); check(first_boot.boot());
    check(first_boot.mode() == Engine::Mode::Trial && swapped(trial, original));
    check(!first_boot.request_update());
    cuts(trial, [](Memory &m) {
        Engine e(m); check(e.boot()); // Unconfirmed image must revert.
    }, [&](Memory &m, Engine &e) {
        check(e.mode() == Engine::Mode::Idle && unchanged(m, original));
    });

    cuts(trial, [](Memory &m) {
        Engine e(m); check(e.load()); check(e.confirm());
    }, [&](Memory &m, Engine &e) {
        check(e.mode() == Engine::Mode::Idle);
        check(swapped(m, original) || unchanged(m, original));
    });

    auto corrupt = original;
    std::fill(corrupt.bytes.begin() + 7 * 64, corrupt.bytes.end(), uint8_t{0});
    Engine damaged(corrupt);
    check(!damaged.boot()); check(!damaged.provision());
    std::cout << "boot swap lab tests passed\n";
}
