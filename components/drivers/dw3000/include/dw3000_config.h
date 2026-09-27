#pragma once

#include <stdint.h>

namespace dw3000 {

struct Config {
    int channel = 5;
    int data_rate = 6800;
    int preamble_length = 128;
    int preamble_code = 9;
    int rx_pac = 8;
    int sfd_type = 1;
    uint16_t sfd_timeout = 129;
    bool sts_enabled = false;
    uint32_t tx_power = 0xfdfdfdfd;
    // Initial estimates only; calibrate each physical board before claiming accuracy.
    uint16_t tx_antenna_delay = 16385U;
    uint16_t rx_antenna_delay = 16385U;
};

}  // namespace dw3000
