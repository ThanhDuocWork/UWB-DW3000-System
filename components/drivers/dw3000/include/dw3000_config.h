#pragma once

namespace dw3000 {

struct Config {
    int channel = 5;
    int data_rate = 6800;
    int preamble_length = 128;
    bool sts_enabled = false;
};

}  // namespace dw3000
