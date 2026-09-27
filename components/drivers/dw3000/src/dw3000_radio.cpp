#include "dw3000_driver.h"

#include "dw3000_hal.h"
#include "dw3000_registers.h"
#include "uwb_log.h"

namespace dw3000 {

static const char *TAG = "dw3000";

namespace {

constexpr uint32_t kTxTimeoutUs = 200000U;
constexpr uint32_t kRxPollIntervalMs = 1U;
bool s_rx_after_tx_pending = false;

bool wait_for_status(uint32_t expected_mask, uint32_t error_mask, uint32_t timeout_us, uint32_t *status_out)
{
    const uint64_t start_us = dw3000_hal::now_us();
    uint32_t status = 0U;

    while ((dw3000_hal::now_us() - start_us) < timeout_us) {
        if (!read_sys_status(&status)) {
            return false;
        }

        if ((status & error_mask) != 0U) {
            if (status_out != nullptr) {
                *status_out = status;
            }
            return false;
        }

        if ((status & expected_mask) == expected_mask) {
            if (status_out != nullptr) {
                *status_out = status;
            }
            return true;
        }

        dw3000_hal::delay_ms(kRxPollIntervalMs);
    }

    if (status_out != nullptr) {
        *status_out = status;
    }
    return false;
}

}  // namespace

bool transmit(const uint8_t *data, size_t size, const TxOptions &options, uint64_t *tx_timestamp)
{
    if (tx_timestamp != nullptr) {
        *tx_timestamp = 0U;
    }
    if (data == nullptr || size == 0U) {
        UWB_LOGE(TAG, "DW3000 TX invalid buffer");
        return false;
    }

    if (size > 127U - registers::FCS_LEN) {
        UWB_LOGE(TAG, "DW3000 TX too long len=%u", static_cast<unsigned>(size));
        return false;
    }

    s_rx_after_tx_pending = false;
    if (!issue_command(registers::CMD_TXRXOFF) ||
        !clear_sys_status(sys_status_tx_done_mask() | sys_status_rx_good_mask() |
                          sys_status_rx_error_mask() |
                          registers::SYS_STATUS_CIADONE_BIT_MASK |
                          registers::SYS_STATUS_HPDWARN_BIT_MASK)) {
        return false;
    }
    if (options.response_expected &&
        (!set_rx_after_tx_delay(options.rx_after_tx_uus) ||
         !set_rx_timeout(options.rx_timeout_uus))) {
        return false;
    }
    uint64_t expected_tx_timestamp = 0U;
    if (options.delayed) {
        if (!predict_delayed_tx_timestamp(options.delayed_time_high32, &expected_tx_timestamp) ||
            !set_delayed_tx_time(options.delayed_time_high32)) {
            return false;
        }
    }
    if (!write_reg(registers::TX_BUFFER, 0U, data, size)) {
        UWB_LOGE(TAG, "DW3000 TX buffer write failed len=%u", static_cast<unsigned>(size));
        return false;
    }

    uint32_t tx_fctrl = 0U;
    if (!read_reg_u32(registers::TX_FCTRL, registers::NO_SUB_ADDRESS, &tx_fctrl)) {
        UWB_LOGE(TAG, "DW3000 TX failed to read frame control");
        return false;
    }

    const uint32_t tx_frame_length = static_cast<uint32_t>(size + registers::FCS_LEN);
    tx_fctrl &= ~(registers::TX_FCTRL_TXB_OFFSET_BIT_MASK |
                  registers::TX_FCTRL_TR_BIT_MASK |
                  registers::TX_FCTRL_TXFLEN_BIT_MASK);
    tx_fctrl |= tx_frame_length & registers::TX_FCTRL_TXFLEN_BIT_MASK;

    if (!write_reg_u32(registers::TX_FCTRL, 0U, tx_fctrl)) {
        UWB_LOGE(TAG, "DW3000 TX frame control write failed len=%u", static_cast<unsigned>(size));
        return false;
    }

    const uint32_t command = options.delayed
        ? (options.response_expected ? registers::CMD_DTX_W4R : registers::CMD_DTX)
        : (options.response_expected ? registers::CMD_TX_W4R : registers::CMD_TX);
    if (!issue_command(command)) {
        (void)issue_command(registers::CMD_TXRXOFF);
        UWB_LOGE(TAG, "DW3000 TX start command failed");
        return false;
    }

    uint32_t status = 0U;
    if (options.delayed) {
        uint32_t state = 0U;
        if (!read_sys_status(&status) || !read_sys_state(&state) ||
            (status & registers::SYS_STATUS_HPDWARN_BIT_MASK) != 0U ||
            state == registers::DW_SYS_STATE_TXERR) {
            (void)issue_command(registers::CMD_TXRXOFF);
            UWB_LOGE(TAG, "Delayed TX rejected status=0x%08lx state=0x%08lx",
                     static_cast<unsigned long>(status), static_cast<unsigned long>(state));
            return false;
        }
    }
    if (!wait_for_status(sys_status_tx_done_mask(), registers::SYS_STATUS_HPDWARN_BIT_MASK,
                         kTxTimeoutUs, &status)) {
        (void)issue_command(registers::CMD_TXRXOFF);
        UWB_LOGE(TAG,
                 "DW3000 TX timeout status=0x%08lx len=%u",
                 static_cast<unsigned long>(status),
                 static_cast<unsigned>(size));
        return false;
    }

    uint64_t actual_tx_timestamp = 0U;
    if (!read_tx_timestamp(&actual_tx_timestamp) ||
        !clear_sys_status(sys_status_tx_done_mask())) {
        (void)issue_command(registers::CMD_TXRXOFF);
        return false;
    }
    if (options.delayed) {
        const uint64_t delta = (actual_tx_timestamp - expected_tx_timestamp) &
                              registers::DW_TIME_40_BIT_MASK;
        if (delta != 0U) {
            (void)issue_command(registers::CMD_TXRXOFF);
            UWB_LOGE(TAG, "Delayed TX timestamp mismatch expected=0x%010llx actual=0x%010llx delta_dtu=%llu",
                     static_cast<unsigned long long>(expected_tx_timestamp),
                     static_cast<unsigned long long>(actual_tx_timestamp),
                     static_cast<unsigned long long>(delta));
            return false;
        }
        UWB_LOGI(TAG, LOG_FLAG_TX, "Delayed TX verified ts=0x%010llx delta_dtu=0",
                 static_cast<unsigned long long>(actual_tx_timestamp));
    }
    if (tx_timestamp != nullptr) {
        *tx_timestamp = actual_tx_timestamp;
    }
    s_rx_after_tx_pending = options.response_expected;
    UWB_LOGI(TAG,
             LOG_FLAG_TX,
             "DW3000 TX sent len=%u status=0x%08lx",
             static_cast<unsigned>(size),
             static_cast<unsigned long>(status));
    return true;
}

bool start_receive()
{
    s_rx_after_tx_pending = false;
    const uint32_t rx_clear_mask =
        registers::SYS_STATUS_RXPRD_BIT_MASK |
        registers::SYS_STATUS_RXSFDD_BIT_MASK |
        registers::SYS_STATUS_CIADONE_BIT_MASK |
        registers::SYS_STATUS_RXPHD_BIT_MASK |
        registers::SYS_STATUS_RXFR_BIT_MASK |
        registers::SYS_STATUS_RXFCG_BIT_MASK |
        registers::SYS_STATUS_ALL_RX_ERR;

    if (!issue_command(registers::CMD_TXRXOFF) ||
        !set_rx_timeout(0U) ||
        !clear_sys_status(sys_status_tx_done_mask() | rx_clear_mask)) {
        return false;
    }

    if (!issue_command(registers::CMD_RX)) {
        UWB_LOGE(TAG, "DW3000 RX start command failed");
        return false;
    }

    return true;
}

bool receive(uint8_t *data, size_t buffer_size, size_t *out_size, uint32_t timeout_ms,
             uint64_t *rx_timestamp, RxMode mode)
{
    if (rx_timestamp != nullptr) {
        *rx_timestamp = 0U;
    }
    if (data == nullptr || out_size == nullptr || buffer_size == 0U) {
        s_rx_after_tx_pending = false;
        (void)issue_command(registers::CMD_TXRXOFF);
        UWB_LOGE(TAG, "DW3000 RX invalid buffer arguments");
        return false;
    }

    *out_size = 0U;

    if (timeout_ms == 0U || timeout_ms > UINT32_MAX / 1000U) {
        s_rx_after_tx_pending = false;
        (void)issue_command(registers::CMD_TXRXOFF);
        return false;
    }
    if (mode == RxMode::AfterTx && !s_rx_after_tx_pending) {
        UWB_LOGE(TAG, "RX-after-TX requested without a pending TX/W4R");
        return false;
    }
    if (mode == RxMode::StartNow && !start_receive()) {
        return false;
    }
    // AfterTx must preserve the hardware RX window and any already-received frame.
    s_rx_after_tx_pending = false;

    uint32_t status = 0U;
    const uint32_t expected_mask = sys_status_rx_good_mask() |
                                  registers::SYS_STATUS_CIADONE_BIT_MASK;
    const uint32_t error_mask = sys_status_rx_error_mask();
    if (!wait_for_status(expected_mask, error_mask, timeout_ms * 1000U, &status)) {
        if (sys_status_has_rx_error(status)) {
            (void)clear_sys_status(status & error_mask);
            (void)issue_command(registers::CMD_TXRXOFF);
            UWB_LOGE(TAG, "DW3000 RX error status=0x%08lx", static_cast<unsigned long>(status));
        } else {
            (void)issue_command(registers::CMD_TXRXOFF);
            UWB_LOGW(TAG, LOG_FLAG_RX, "RX software timeout status=0x%08lx",
                     static_cast<unsigned long>(status));
        }
        return false;
    }

    uint64_t timestamp = 0U;
    if (!read_rx_timestamp(&timestamp)) {
        (void)issue_command(registers::CMD_TXRXOFF);
        return false;
    }
    uint32_t rx_finfo = 0U;
    if (!read_reg_u32(registers::RX_FINFO, 0U, &rx_finfo)) {
        (void)issue_command(registers::CMD_TXRXOFF);
        UWB_LOGE(TAG, "DW3000 RX failed to read frame info");
        return false;
    }

    const size_t frame_len = static_cast<size_t>(rx_finfo & registers::RX_FINFO_RXFLEN_BIT_MASK);
    if (frame_len <= registers::FCS_LEN) {
        (void)issue_command(registers::CMD_TXRXOFF);
        UWB_LOGE(TAG, "DW3000 RX invalid frame length=%u", static_cast<unsigned>(frame_len));
        return false;
    }

    const size_t payload_len = frame_len - registers::FCS_LEN;
    if (payload_len > buffer_size) {
        (void)issue_command(registers::CMD_TXRXOFF);
        UWB_LOGE(TAG,
                 "DW3000 RX frame too large payload=%u buffer=%u",
                 static_cast<unsigned>(payload_len),
                 static_cast<unsigned>(buffer_size));
        return false;
    }

    if (!read_reg(registers::RX_BUFFER_0, 0U, data, payload_len)) {
        (void)issue_command(registers::CMD_TXRXOFF);
        UWB_LOGE(TAG, "DW3000 RX buffer read failed len=%u", static_cast<unsigned>(payload_len));
        return false;
    }

    if (!clear_sys_status(expected_mask)) {
        (void)issue_command(registers::CMD_TXRXOFF);
        return false;
    }
    *out_size = payload_len;
    if (rx_timestamp != nullptr) {
        *rx_timestamp = timestamp;
    }
    UWB_LOGI(TAG,
             LOG_FLAG_RX,
             "DW3000 RX good frame len=%u status=0x%08lx",
             static_cast<unsigned>(payload_len),
             static_cast<unsigned long>(status));
    return true;
}

}  // namespace dw3000
