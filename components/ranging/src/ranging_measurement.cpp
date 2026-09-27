#include "ranging_measurement.h"

namespace ranging {

Measurement calculate_measurement(const Timestamps &t)
{
    Measurement result{};
    if ((t.t1 | t.t2 | t.t3 | t.t4 | t.t5 | t.t6) > TIME_MASK) {
        return result;
    }
    const uint64_t ra_ticks = (t.t4 - t.t1) & TIME_MASK;
    const uint64_t rb_ticks = (t.t6 - t.t3) & TIME_MASK;
    const uint64_t da_ticks = (t.t5 - t.t4) & TIME_MASK;
    const uint64_t db_ticks = (t.t3 - t.t2) & TIME_MASK;
    constexpr uint64_t max_interval = RX_TIMEOUT_UUS * UUS_TO_DTU;
    const uint64_t intervals[] = {ra_ticks, rb_ticks, da_ticks, db_ticks};
    for (const auto ticks : intervals) {
        if (ticks == 0U || ticks > max_interval) {
            return result;
        }
    }
    const double ra = static_cast<double>(ra_ticks);
    const double rb = static_cast<double>(rb_ticks);
    const double da = static_cast<double>(da_ticks);
    const double db = static_cast<double>(db_ticks);
    // APS013 asymmetric DS-TWR. Cast before multiplication to avoid integer overflow.
    result.tof_dtu = (ra * rb - da * db) / (ra + rb + da + db);
    constexpr double seconds_per_tick = 1.0 / (499.2e6 * 128.0);
    constexpr double speed_of_light_air = 299702547.0;
    result.distance_m = result.tof_dtu * seconds_per_tick * speed_of_light_air;
    // Retain rejected values for calibration diagnostics, but never mark them valid.
    // Ordered bounds also reject NaN and infinities.
    result.valid = result.distance_m >= 0.0 && result.distance_m <= 1000.0;
    return result;
}

}  // namespace ranging
