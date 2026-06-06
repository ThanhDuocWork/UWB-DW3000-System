#pragma once

namespace ranging {

struct Measurement {
    float distance_m = 0.0f;
    float rssi_dbm = 0.0f;
    bool valid = false;
};

}  // namespace ranging
