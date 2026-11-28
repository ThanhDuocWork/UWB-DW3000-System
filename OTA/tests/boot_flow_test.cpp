#include "boot_flow.h"
#include "swap_core.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

using Engine = boot_swap::Core<64, 3>;
using Version = boot_swap::PrimaryVersion;
using Status = Engine::JournalStatus;
using Mode = Engine::Mode;
static void check(bool ok) { if (!ok) throw std::runtime_error("boot flow invariant"); }
struct Cut {};
struct Memory : Engine::Flash {
    std::array<uint8_t, Engine::sector_count * 64> bytes{};
    int cut = -1, mutations = 0, reads = 0, fail_read = -1;
    Memory() { bytes.fill(0xff); }
    bool read(unsigned s, unsigned o, void *p, unsigned n) override {
        if (reads++ == fail_read) return false;
        std::memcpy(p, bytes.data() + s * 64 + o, n); return true;
    }
    void touch() { if (mutations == cut) throw Cut{}; ++mutations; }
    bool erase(unsigned s) override {
        for (unsigned i = 0; i < 64; ++i) { touch(); bytes[s * 64 + i] = 0xff; }
        return true;
    }
    bool write(unsigned s, unsigned o, const void *p, unsigned n) override {
        const auto *src = static_cast<const uint8_t *>(p);
        for (unsigned i = 0; i < n; ++i) {
            touch(); auto &dest = bytes[s * 64 + o + i];
            check((dest & src[i]) == src[i]); dest &= src[i];
        }
        return true;
    }
    void reset(int point = -1) { cut = point; mutations = reads = 0; fail_read = -1; }
};
static Memory images() {
    Memory m;
    std::fill_n(m.bytes.begin(), 192, uint8_t{0xa1});
    std::fill_n(m.bytes.begin() + 192, 192, uint8_t{0xb1});
    return m;
}
static bool primary(const Memory &m, Version v) {
    const auto is = [&](uint8_t b) {
        return std::all_of(m.bytes.begin(), m.bytes.begin() + 192,
                           [b](uint8_t a) { return a == b; });
    };
    return (v != Version::B && is(0xa1)) || (v != Version::A && is(0xb1));
}
static bool run(Memory &m) {
    Engine core(m);
    return boot_swap::boot_flow(core, [&](Version v) {
        // Any verification of a mixed slot fails, including before recovery.
        check(primary(m, Version::Either));
        return primary(m, v);
    });
}
struct Fake {
    using JournalStatus = Status;
    using Mode = Engine::Mode;
    Status status = Status::Valid;
    Mode initial = Mode::Idle, final = Mode::Idle;
    int boots = 0;
    bool succeeds = true;
    Status load_status() { return status; }
    Mode mode() const { return boots ? final : initial; }
    bool boot() { ++boots; return succeeds; }
    bool reject_ready() { return false; }
};
static void policies() {
    for (auto status : {Status::Blank, Status::Corrupt, Status::IoError}) {
        Fake f; f.status = status; int calls = 0;
        check(boot_swap::boot_flow(f, [&](Version v) {
            ++calls; check(v == Version::A); return true;
        }) == (status == Status::Blank));
        check(f.boots == 0 && calls == (status == Status::Blank ? 1 : 0));
    }
    for (auto mode : {Mode::Idle, Mode::Forward, Mode::Ready, Mode::Trial, Mode::Reverse}) {
        Fake f; f.initial = mode;
        f.final = mode == Mode::Forward || mode == Mode::Ready ? Mode::Trial : Mode::Idle;
        int calls = 0;
        check(boot_swap::boot_flow(f, [&](Version v) {
            ++calls;
            if (!f.boots) check(mode == Mode::Ready && v == Version::B);
            else check(v == (f.final == Mode::Trial ? Version::B :
                             mode == Mode::Idle ? Version::Either : Version::A));
            return true;
        }));
        check(f.boots == 1 && calls == (mode == Mode::Ready ? 2 : 1));
    }
    Fake ready; ready.initial = Mode::Ready;
    check(!boot_swap::boot_flow(ready, [](Version) { return false; }) && ready.boots == 0);
    Fake post; post.initial = Mode::Forward; post.final = Mode::Trial;
    check(!boot_swap::boot_flow(post, [](Version) { return false; }) && post.boots == 1);
    Fake failure; failure.succeeds = false;
    check(!boot_swap::boot_flow(failure, [](Version) { check(false); return true; }));
    Fake wrong; wrong.initial = Mode::Forward;
    check(!boot_swap::boot_flow(wrong, [](Version) { check(false); return true; }));
}
static void journal_cases() {
    Memory blank = images(); Engine b(blank);
    check(b.load_status() == Status::Blank);
    blank.bytes[192] = 0; // Secondary is not required for commissioning.
    check(run(blank) && blank.mutations == 0);
    blank.bytes[0] = 0xb1;
    check(!boot_swap::boot_flow(b, [&](Version v) { return primary(blank, v); }));
    Memory corrupt = images(); corrupt.bytes.back() = 0;
    Engine c(corrupt); check(c.load_status() == Status::Corrupt);
    check(!run(corrupt) && corrupt.mutations == 0);
    Memory io = images(); io.fail_read = 3;
    Engine e(io); check(e.load_status() == Status::IoError && io.reads == 4);
    io.reset(); check(e.provision()); io.reset(); io.fail_read = 3;
    check(e.load_status() == Status::IoError && io.reads == 4);
    io.reset(); io.bytes[7 * 64 + 32] = 0; // Torn record beside last valid.
    check(e.load_status() == Status::Valid);
    Mode mode{}; uint32_t seq = 0;
    check(Engine::inspect_journal(io, mode, seq) && mode == Mode::Idle && seq == 1);
    io.reset(); io.fail_read = 3;
    check(!run(io) && io.mutations == 0);
}
static void cuts(const Memory &start, bool reverse) {
    auto baseline = start; baseline.reset(); Engine full(baseline); check(full.boot());
    const int total = baseline.mutations;
    for (int i = 0; i <= total; ++i) {
        auto m = start; m.reset(i); Engine interrupted(m);
        try { check(interrupted.boot()); } catch (const Cut &) {}
        m.reset(); check(run(m));
        Engine final(m); check(final.load());
        check(primary(m, reverse || final.mode() == Mode::Idle ? Version::A : Version::B));
    }
    std::cout << (reverse ? "reverse" : "forward") << " recovery: " << total + 1
              << " byte-mutation cuts passed\n";
}
static void ready_bad_b_recovers_a() {
    auto start = images();
    Engine setup(start);
    check(setup.provision() && setup.request_update());
    auto full = start; full.reset(); Engine forward(full); check(forward.boot());
    for (int point = 0; point < full.mutations; ++point) {
        auto m = start; m.reset(point); Engine interrupted(m);
        try { check(interrupted.boot()); } catch (const Cut &) {}
        m.reset(); Engine probe(m); check(probe.load());
        if (probe.mode() != Mode::Ready) continue;
        check(primary(m, Version::B));
        check(std::equal(start.bytes.begin(), start.bytes.begin() + 192, m.bytes.begin() + 192));
        m.bytes[100] ^= 1;
        for (bool damage_a : {false, true}) {
            auto test = m;
            if (damage_a) test.bytes[192 + 100] ^= 1;
            for (int reset = 0; reset < 2; ++reset) {
                Engine boot(test);
                const bool allowed = boot_swap::boot_flow(boot, [&](Version v) {
                    return primary(test, v);
                });
                if (allowed == damage_a || (!damage_a && !primary(test, Version::A)))
                    throw std::runtime_error("Ready with invalid B: unexpected recovery outcome");
                if (!damage_a) check(boot.load() && boot.mode() == Mode::Idle);
            }
        }
        return;
    }
    throw std::runtime_error("no durable Ready checkpoint found");
}
int main() try {
    ready_bad_b_recovers_a();
    policies(); journal_cases();
    auto m = images(); Engine core(m); check(core.provision());
    check(core.request_update()); cuts(m, false);
    check(run(m)); check(primary(m, Version::B));
    auto trial = m; cuts(trial, true);
    Engine confirm(m); check(confirm.load()); check(confirm.confirm());
    // Confirmed B boots with invalid secondary; no slot verification gate.
    std::fill_n(m.bytes.begin() + 192, 192, uint8_t{0}); m.reset();
    check(run(m) && primary(m, Version::B) && m.mutations == 0);
    // Post-recovery verification failure cannot allow execution; next reset rolls back.
    auto bad = images(); bad.bytes[192 + 100] = 0; Engine pending(bad);
    check(pending.provision()); check(pending.request_update());
    check(!boot_swap::boot_flow(pending, [&](Version v) { return primary(bad, v); }));
    check(pending.mode() == Mode::Trial);
    check(run(bad) && primary(bad, Version::A));
    std::cout << "boot flow policy, journal, confirm and rollback tests passed\n";
    return 0;
} catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
}
