#include "ranging_measurement.h"

static bool near(double a, double b, double tolerance = 0.001)
{
    const double difference = a - b;
    return difference >= -tolerance && difference <= tolerance;
}

static ranging::Timestamps sample(uint64_t origin = 0U)
{
    // Propagation=1000 ticks, anchor reply=3e9, tag reply=2e9 ticks.
    // Independent clock origins must not affect the result.
    ranging::Timestamps t{};
    t.t1 = origin & ranging::TIME_MASK;
    t.t2 = (origin + 9000000U + 1000U) & ranging::TIME_MASK;
    t.t3 = (t.t2 + 3000000000ULL) & ranging::TIME_MASK;
    t.t4 = (origin + 3000002000ULL) & ranging::TIME_MASK;
    t.t5 = (t.t4 + 2000000000ULL) & ranging::TIME_MASK;
    t.t6 = (t.t3 + 2000002000ULL) & ranging::TIME_MASK;
    return t;
}

int main()
{
    const auto good = ranging::calculate_measurement(sample());
    if (!good.valid || !near(good.tof_dtu, 1000.0) ||
        !near(good.distance_m, 4.6903568679)) return 1;
    const auto wrap = ranging::calculate_measurement(sample(ranging::TIME_MASK - 1000000U));
    if (!wrap.valid || !near(wrap.distance_m, good.distance_m)) return 2;
    if (ranging::calculate_measurement({}).valid) return 3;
    auto bad = sample();
    bad.t4 = bad.t1; // Zero interval.
    if (ranging::calculate_measurement(bad).valid) return 4;
    bad = sample();
    bad.t6 = (bad.t3 - 1U) & ranging::TIME_MASK; // Reversed / stale timestamp.
    if (ranging::calculate_measurement(bad).valid) return 5;
    bad = sample();
    bad.t1 = ranging::TIME_MASK + 1U;
    if (ranging::calculate_measurement(bad).valid) return 6;
    bad = sample();
    bad.t4 -= 10000U; // Negative ToF.
    bad.t5 -= 10000U;
    if (ranging::calculate_measurement(bad).valid) return 7;
    bad = sample();
    bad.t4 += 1000000U;
    bad.t5 += 1000000U;
    bad.t6 += 1000000U; // Implausible range.
    if (ranging::calculate_measurement(bad).valid) return 8;

    // Anchor clock runs 10 ppm faster; DS-TWR should remain close to the true ToF.
    auto skew = sample();
    skew.t3 = skew.t2 + 3000030000ULL;
    skew.t6 = skew.t3 + 2000022000ULL;
    const auto skewed = ranging::calculate_measurement(skew);
    if (!skewed.valid || !near(skewed.tof_dtu, 1000.0, 0.1)) return 9;

    ranging::Session session{};
    session.time = sample();
    session.phase = ranging::Phase::Complete;
    ranging::reset_session(session, 255U, 100U);
    if (session.time.t3 != 0U || session.phase != ranging::Phase::Idle ||
        session.sequence != 255U || session.peer != 100U) return 10;
    session.time = sample();
    ranging::fail_session(session);
    if (session.phase != ranging::Phase::Failed || session.time.t1 != 0U ||
        session.time.t2 != 0U || session.time.t3 != 0U || session.time.t4 != 0U ||
        session.time.t5 != 0U || session.time.t6 != 0U) return 11;
    ranging::reset_session(session, static_cast<uint8_t>(session.sequence + 1U), 100U);
    if (session.sequence != 0U) return 12;
    return 0;
}
