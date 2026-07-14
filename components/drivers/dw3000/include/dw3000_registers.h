#pragma once

#include <stddef.h>
#include <stdint.h>

namespace dw3000::registers {

constexpr uint16_t NO_SUB_ADDRESS = 0U;
constexpr size_t DEV_ID_LEN = 4U;
constexpr uint32_t DEV_ID = 0x000000U;
constexpr uint32_t CHAN_CTRL = 0x010014U;
constexpr uint32_t TX_FCTRL = 0x000024U;
constexpr uint32_t TX_FCTRL_HI = 0x000028U;
constexpr uint32_t SYS_STATUS = 0x000044U;
constexpr uint32_t RX_FINFO = 0x00004CU;
constexpr uint32_t DTUNE0 = 0x060000U;
constexpr uint32_t SYS_STATE = 0x0F0030U;
constexpr uint32_t RX_BUFFER_0 = 0x120000U;
constexpr uint32_t RX_BUFFER_1 = 0x130000U;
constexpr uint32_t TX_BUFFER = 0x140000U;

constexpr size_t RX_BUFFER_MAX_LEN = 1023U;
constexpr size_t TX_BUFFER_MAX_LEN = 1024U;
constexpr uint32_t REG_DIRECT_OFFSET_MAX_LEN = 127U;
constexpr uint32_t FCS_LEN = 2U;

constexpr uint32_t TX_FCTRL_TXFLEN_BIT_MASK = 0x000003FFUL;
constexpr uint32_t TX_FCTRL_TXBR_BIT_OFFSET = 10U;
constexpr uint32_t TX_FCTRL_TXBR_BIT_MASK = 0x00000400UL;
constexpr uint32_t TX_FCTRL_TR_BIT_OFFSET = 11U;
constexpr uint32_t TX_FCTRL_TR_BIT_MASK = 0x00000800UL;
constexpr uint32_t TX_FCTRL_TXPSR_BIT_OFFSET = 12U;
constexpr uint32_t TX_FCTRL_TXPSR_BIT_MASK = 0x0000F000UL;
constexpr uint32_t TX_FCTRL_TXB_OFFSET_BIT_OFFSET = 16U;
constexpr uint32_t TX_FCTRL_TXB_OFFSET_BIT_MASK = 0x03FF0000UL;

constexpr uint32_t CHAN_CTRL_RX_PCODE_BIT_OFFSET = 8U;
constexpr uint32_t CHAN_CTRL_RX_PCODE_BIT_MASK = 0x00001F00UL;
constexpr uint32_t CHAN_CTRL_TX_PCODE_BIT_OFFSET = 3U;
constexpr uint32_t CHAN_CTRL_TX_PCODE_BIT_MASK = 0x000000F8UL;
constexpr uint32_t CHAN_CTRL_SFD_TYPE_BIT_OFFSET = 1U;
constexpr uint32_t CHAN_CTRL_SFD_TYPE_BIT_MASK = 0x00000006UL;
constexpr uint32_t CHAN_CTRL_RF_CHAN_BIT_OFFSET = 0U;
constexpr uint32_t CHAN_CTRL_RF_CHAN_BIT_MASK = 0x00000001UL;

constexpr uint8_t DTUNE0_PRE_PAC_SYM_BIT_OFFSET = 0U;
constexpr uint8_t DTUNE0_PRE_PAC_SYM_BIT_MASK = 0x03U;

constexpr uint8_t DWT_PAC8 = 0U;
constexpr uint8_t DWT_PAC16 = 1U;
constexpr uint8_t DWT_PAC32 = 2U;
constexpr uint8_t DWT_PAC64 = 3U;
constexpr uint8_t DWT_PLEN_128 = 0x05U;
constexpr uint8_t DWT_BR_6M8 = 0x01U;
constexpr uint16_t DWT_SFDTOC_DEF = 129U;

// Bring-up / boot
constexpr uint32_t SYS_STATUS_ARFE_BIT_MASK = 0x20000000UL;
constexpr uint32_t SYS_STATUS_RXSTO_BIT_MASK = 0x04000000UL;
constexpr uint32_t SYS_STATUS_RCINIT_BIT_MASK = 0x01000000UL;
constexpr uint32_t SYS_STATUS_SPIRDY_BIT_MASK = 0x00800000UL;
constexpr uint32_t SYS_STATUS_CPLOCK_BIT_MASK = 0x00000002UL;
constexpr uint32_t SYS_STATUS_CP_LOCK_BIT_MASK = SYS_STATUS_CPLOCK_BIT_MASK;

// RX timeout / errors
constexpr uint32_t SYS_STATUS_RXPTO_BIT_MASK = 0x00200000UL;
constexpr uint32_t SYS_STATUS_RXOVRR_BIT_MASK = 0x00100000UL;
constexpr uint32_t SYS_STATUS_VWARN_BIT_MASK = 0x00080000UL;
constexpr uint32_t SYS_STATUS_CIAERR_BIT_MASK = 0x00040000UL;
constexpr uint32_t SYS_STATUS_RXFTO_BIT_MASK = 0x00020000UL;
constexpr uint32_t SYS_STATUS_RXFSL_BIT_MASK = 0x00010000UL;
constexpr uint32_t SYS_STATUS_RXFCE_BIT_MASK = 0x00008000UL;

// RX progress
constexpr uint32_t SYS_STATUS_RXFCG_BIT_MASK = 0x00004000UL;
constexpr uint32_t SYS_STATUS_RXFR_BIT_MASK = 0x00002000UL;
constexpr uint32_t SYS_STATUS_RXPHE_BIT_MASK = 0x00001000UL;
constexpr uint32_t SYS_STATUS_RXPHD_BIT_MASK = 0x00000800UL;
constexpr uint32_t SYS_STATUS_CIADONE_BIT_MASK = 0x00000400UL;
constexpr uint32_t SYS_STATUS_RXSFDD_BIT_MASK = 0x00000200UL;
constexpr uint32_t SYS_STATUS_RXPRD_BIT_MASK = 0x00000100UL;

// TX progress
constexpr uint32_t SYS_STATUS_TXFRS_BIT_MASK = 0x00000080UL;
constexpr uint32_t SYS_STATUS_TXPHS_BIT_MASK = 0x00000040UL;
constexpr uint32_t SYS_STATUS_TXPRS_BIT_MASK = 0x00000020UL;
constexpr uint32_t SYS_STATUS_TXFRB_BIT_MASK = 0x00000010UL;

constexpr uint32_t SYS_STATUS_ALL_RX_ERR =
    SYS_STATUS_ARFE_BIT_MASK |
    SYS_STATUS_RXPTO_BIT_MASK |
    SYS_STATUS_RXOVRR_BIT_MASK |
    SYS_STATUS_VWARN_BIT_MASK |
    SYS_STATUS_CIAERR_BIT_MASK |
    SYS_STATUS_RXFTO_BIT_MASK |
    SYS_STATUS_RXSTO_BIT_MASK |
    SYS_STATUS_RXFSL_BIT_MASK |
    SYS_STATUS_RXFCE_BIT_MASK |
    SYS_STATUS_RXPHE_BIT_MASK;

constexpr uint32_t DW_SYS_STATE_TXERR = 0x000D0000UL;

constexpr uint32_t RX_FINFO_RXFLEN_BIT_MASK = 0x000003FFUL;

constexpr uint32_t CMD_TXRXOFF = 0x0U;
constexpr uint32_t CMD_TX = 0x1U;
constexpr uint32_t CMD_RX = 0x2U;

}  // namespace dw3000::registers
