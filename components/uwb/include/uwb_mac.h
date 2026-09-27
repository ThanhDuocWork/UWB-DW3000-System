#pragma once

#include <stdint.h>

#include "uwb_frame.h"
#include "dw3000_driver.h"

namespace uwb {

bool send(const Frame &frame, const dw3000::TxOptions &options = {},
          uint64_t *tx_timestamp = nullptr);
bool receive(Frame &frame, uint32_t timeout_ms, uint64_t *rx_timestamp = nullptr,
             dw3000::RxMode mode = dw3000::RxMode::StartNow);

}  // namespace uwb
