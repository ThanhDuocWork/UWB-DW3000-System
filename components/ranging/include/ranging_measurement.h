#pragma once
#include "ranging_session.h"

namespace ranging {

struct Measurement {
    double distance_m = 0.0;
    double tof_dtu = 0.0;
    bool valid = false;
};

// Timestamp values may be zero or wrap; validate elapsed intervals, not absolutes.
Measurement calculate_measurement(const Timestamps &time);

}  // namespace ranging
