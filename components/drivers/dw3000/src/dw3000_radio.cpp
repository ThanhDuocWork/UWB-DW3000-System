#include "dw3000_driver.h"

#include <algorithm>

#include "dw3000_hal.h"
#include "dw3000_registers.h"
#include "uwb_log.h"

namespace dw3000 {

static const char *TAG = "dw3000";

namespace {

constexpr uint32_t kTxTimeoutUs = 200000U;
constexpr uint32_t kRxPollIntervalMs = 1U;

bool wait_for_status(uint32_t expected_mask, uint32_t error_mask, uint32_t timeout_us, uint32_t *status_out)
{
    const uint64_t start_us = dw3000_hal::now_us();
    uint32_t status = 0U;

    while ((dw3000_hal::now_us() - start_us) < timeout_us) {
        if (!read_reg_u32(registers::SYS_STATUS, registers::NO_SUB_ADDRESS, &status)) {
            return false;
        }

        if ((status & expected_mask) != 0U) {
            if (status_out != nullptr) {
                *status_out = status;
            }
            return true;
        }

        if ((status & error_mask) != 0U) {
            if (status_out != nullptr) {
                *status_out = status;
            }
            return false;
        }

        dw3000_hal::delay_ms(kRxPollIntervalMs);
    }

    if (status_out != nullptr) {
        *status_out = status;
    }
    return false;
}

}  // namespace

bool transmit(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0U) {
        UWB_LOGE(TAG, "DW3000 TX invalid buffer");
        return false;
    }

    if ((size + registers::FCS_LEN) > registers::TX_BUFFER_MAX_LEN) {
        UWB_LOGE(TAG, "DW3000 TX too long len=%u", static_cast<unsigned>(size));
        return false;
    }

    if (!write_reg(registers::TX_BUFFER, 0U, data, size)) {
        UWB_LOGE(TAG, "DW3000 TX buffer write failed len=%u", static_cast<unsigned>(size));
        return false;
    }

    const uint32_t tx_frame_length = static_cast<uint32_t>(size + registers::FCS_LEN);
    const uint32_t tx_fctrl = tx_frame_length & registers::TX_FCTRL_TXFLEN_BIT_MASK;
    if (!write_reg_u32(registers::TX_FCTRL, 0U, tx_fctrl)) {
        UWB_LOGE(TAG, "DW3000 TX frame control write failed len=%u", static_cast<unsigned>(size));
        return false;
    }

    (void)write_reg_u32(registers::SYS_STATUS, 0U, registers::SYS_STATUS_TXFRS_BIT_MASK);

    if (!issue_command(registers::CMD_TX)) {
        UWB_LOGE(TAG, "DW3000 TX start command failed");
        return false;
    }

    uint32_t status = 0U;
    if (!wait_for_status(registers::SYS_STATUS_TXFRS_BIT_MASK, 0U, kTxTimeoutUs, &status)) {
        UWB_LOGE(TAG,
                 "DW3000 TX timeout status=0x%08lx len=%u",
                 static_cast<unsigned long>(status),
                 static_cast<unsigned>(size));
        return false;
    }

    (void)write_reg_u32(registers::SYS_STATUS, 0U, registers::SYS_STATUS_TXFRS_BIT_MASK);
    UWB_LOGI(TAG,
             LOG_FLAG_TX,
             "DW3000 TX sent len=%u status=0x%08lx",
             static_cast<unsigned>(size),
             static_cast<unsigned long>(status));
    return true;
}

bool start_receive()
{
    (void)write_reg_u32(registers::SYS_STATUS,
                        0U,
                        registers::SYS_STATUS_RXFCG_BIT_MASK |
                            registers::SYS_STATUS_ALL_RX_ERR);

    if (!issue_command(registers::CMD_RX)) {
        UWB_LOGE(TAG, "DW3000 RX start command failed");
        return false;
    }

    UWB_LOGI(TAG, LOG_FLAG_RX, "DW3000 RX enabled");
    return true;
}

bool receive(uint8_t *data, size_t buffer_size, size_t *out_size, uint32_t timeout_ms)
{
    if (data == nullptr || out_size == nullptr || buffer_size == 0U) {
        UWB_LOGE(TAG, "DW3000 RX invalid buffer arguments");
        return false;
    }

    *out_size = 0U;

    if (!start_receive()) {
        return false;
    }

    uint32_t status = 0U;
    const uint32_t expected_mask = registers::SYS_STATUS_RXFCG_BIT_MASK;
    const uint32_t error_mask = registers::SYS_STATUS_ALL_RX_ERR;
    if (!wait_for_status(expected_mask, error_mask, timeout_ms * 1000U, &status)) {
        if ((status & error_mask) != 0U) {
            (void)write_reg_u32(registers::SYS_STATUS, 0U, status & error_mask);
            UWB_LOGE(TAG, "DW3000 RX error status=0x%08lx", static_cast<unsigned long>(status));
        }
        return false;
    }

    uint32_t rx_finfo = 0U;
    if (!read_reg_u32(registers::RX_FINFO, 0U, &rx_finfo)) {
        UWB_LOGE(TAG, "DW3000 RX failed to read frame info");
        return false;
    }

    const size_t frame_len = static_cast<size_t>(rx_finfo & registers::RX_FINFO_RXFLEN_BIT_MASK);
    if (frame_len <= registers::FCS_LEN) {
        (void)write_reg_u32(registers::SYS_STATUS, 0U, registers::SYS_STATUS_RXFCG_BIT_MASK);
        UWB_LOGE(TAG, "DW3000 RX invalid frame length=%u", static_cast<unsigned>(frame_len));
        return false;
    }

    const size_t payload_len = frame_len - registers::FCS_LEN;
    if (payload_len > buffer_size) {
        (void)write_reg_u32(registers::SYS_STATUS, 0U, registers::SYS_STATUS_RXFCG_BIT_MASK);
        UWB_LOGE(TAG,
                 "DW3000 RX frame too large payload=%u buffer=%u",
                 static_cast<unsigned>(payload_len),
                 static_cast<unsigned>(buffer_size));
        return false;
    }

    if (!read_reg(registers::RX_BUFFER_0, 0U, data, payload_len)) {
        UWB_LOGE(TAG, "DW3000 RX buffer read failed len=%u", static_cast<unsigned>(payload_len));
        return false;
    }

    *out_size = payload_len;
    (void)write_reg_u32(registers::SYS_STATUS, 0U, registers::SYS_STATUS_RXFCG_BIT_MASK);
    UWB_LOGI(TAG,
             LOG_FLAG_RX,
             "DW3000 RX good frame len=%u status=0x%08lx",
             static_cast<unsigned>(payload_len),
             static_cast<unsigned long>(status));
    return true;
}

}  // namespace dw3000
