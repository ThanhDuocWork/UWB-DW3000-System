#pragma once

#include <stddef.h>
#include <stdint.h>

#include "dw3000_config.h"
#include "dw3000_types.h"

namespace dw3000 {

enum LogFlag : unsigned int {
    LOG_FLAG_INIT = 1U << 0,
    LOG_FLAG_TX = 1U << 1,
    LOG_FLAG_RX = 1U << 2,
    LOG_FLAG_IRQ_POLL = 1U << 3,
    LOG_FLAG_DIAG = 1U << 4,
    LOG_FLAG_REG = 1U << 5,
};

bool init(const Config &config);
DeviceInfo get_device_info();
bool read_reg(uint32_t reg, uint16_t offset, uint8_t *data, size_t size);
bool write_reg(uint32_t reg, uint16_t offset, const uint8_t *data, size_t size);
bool read_reg_u32(uint32_t reg, uint16_t offset, uint32_t *value);
bool write_reg_u32(uint32_t reg, uint16_t offset, uint32_t value);
bool read_sys_status(uint32_t *status);
bool clear_sys_status(uint32_t mask);
bool read_sys_state(uint32_t *state);
bool sys_status_is_ready_after_boot(uint32_t status);
bool sys_status_has_tx_done(uint32_t status);
bool sys_status_has_rx_good(uint32_t status);
bool sys_status_has_rx_error(uint32_t status);
uint32_t sys_status_tx_done_mask();
uint32_t sys_status_rx_good_mask();
uint32_t sys_status_rx_error_mask();
bool issue_command(uint32_t command);
uint32_t read_device_id();
bool read_rx_timestamp(uint64_t *timestamp);
bool read_tx_timestamp(uint64_t *timestamp);
bool set_delayed_tx_time(uint32_t time_high32);
// Same aligned/antenna-adjusted timestamp used by transmit() verification.
bool predict_delayed_tx_timestamp(uint32_t time_high32, uint64_t *timestamp);
bool set_rx_timeout(uint32_t timeout_uus);
bool set_rx_after_tx_delay(uint32_t delay_uus);
struct TxOptions {
    // Absolute DX_TIME: bits 39:8 of the device clock; bit 0 is ignored.
    // This blocking API times out 200 ms after issuing the TX command.
    bool delayed = false;
    uint32_t delayed_time_high32 = 0;
    bool response_expected = false;
    uint32_t rx_after_tx_uus = 0;
    uint32_t rx_timeout_uus = 0;
};

// StartNow disables hardware RX timeout and uses the software timeout.
// AfterTx consumes the existing W4R window, without restarting or clearing RX.
enum class RxMode { StartNow, AfterTx };

// Single radio owner/task only. Timestamp outputs are valid only on success,
// in 40-bit DW3000 device ticks. RX waits for RXFCG and CIADONE.
bool transmit(const uint8_t *data, size_t size,
              const TxOptions &options = {}, uint64_t *tx_timestamp = nullptr);
bool start_receive();
bool receive(uint8_t *data, size_t buffer_size, size_t *out_size, uint32_t timeout_ms,
             uint64_t *rx_timestamp = nullptr, RxMode mode = RxMode::StartNow);

}  // namespace dw3000
