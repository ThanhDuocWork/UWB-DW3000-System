#pragma once

#include <stddef.h>
#include <stdint.h>

namespace dw3000::registers {

constexpr uint16_t NO_SUB_ADDRESS = 0U;
constexpr size_t DEV_ID_LEN = 4U;
constexpr uint32_t DEV_ID = 0x000000U;
constexpr uint32_t TX_FCTRL = 0x000024U;
constexpr uint32_t SYS_STATUS = 0x000044U;
constexpr uint32_t RX_FINFO = 0x00004CU;
constexpr uint32_t RX_BUFFER_0 = 0x120000U;
constexpr uint32_t RX_BUFFER_1 = 0x130000U;
constexpr uint32_t TX_BUFFER = 0x140000U;

constexpr size_t RX_BUFFER_MAX_LEN = 1023U;
constexpr size_t TX_BUFFER_MAX_LEN = 1024U;
constexpr uint32_t REG_DIRECT_OFFSET_MAX_LEN = 127U;
constexpr uint32_t FCS_LEN = 2U;

constexpr uint32_t TX_FCTRL_TXFLEN_BIT_MASK = 0x000003FFUL;
constexpr uint32_t TX_FCTRL_TR_BIT_OFFSET = 11U;
constexpr uint32_t TX_FCTRL_TR_BIT_MASK = 0x00000800UL;
constexpr uint32_t TX_FCTRL_TXB_OFFSET_BIT_OFFSET = 16U;
constexpr uint32_t TX_FCTRL_TXB_OFFSET_BIT_MASK = 0x03FF0000UL;

constexpr uint32_t SYS_STATUS_ARFE_BIT_MASK = 0x20000000UL;
constexpr uint32_t SYS_STATUS_RXSTO_BIT_MASK = 0x04000000UL;
constexpr uint32_t SYS_STATUS_RXFSL_BIT_MASK = 0x00010000UL;
constexpr uint32_t SYS_STATUS_RXFCE_BIT_MASK = 0x00008000UL;
constexpr uint32_t SYS_STATUS_RXFCG_BIT_MASK = 0x00004000UL;
constexpr uint32_t SYS_STATUS_RXFR_BIT_MASK = 0x00002000UL;
constexpr uint32_t SYS_STATUS_RXPHE_BIT_MASK = 0x00001000UL;
constexpr uint32_t SYS_STATUS_TXFRS_BIT_MASK = 0x00000080UL;

constexpr uint32_t SYS_STATUS_ALL_RX_ERR =
    SYS_STATUS_ARFE_BIT_MASK |
    SYS_STATUS_RXSTO_BIT_MASK |
    SYS_STATUS_RXFSL_BIT_MASK |
    SYS_STATUS_RXFCE_BIT_MASK |
    SYS_STATUS_RXPHE_BIT_MASK;

constexpr uint32_t RX_FINFO_RXFLEN_BIT_MASK = 0x000003FFUL;

constexpr uint32_t CMD_TXRXOFF = 0x0U;
constexpr uint32_t CMD_TX = 0x1U;
constexpr uint32_t CMD_RX = 0x2U;

}  // namespace dw3000::registers
