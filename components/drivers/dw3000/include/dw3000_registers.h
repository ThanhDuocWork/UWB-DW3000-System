#pragma once

#include <stddef.h>
#include <stdint.h>

namespace dw3000::registers {

constexpr uint16_t NO_SUB_ADDRESS = 0U;
constexpr size_t DEV_ID_LEN = 4U;
constexpr uint32_t DEV_ID = 0x000000U;
constexpr uint32_t SYS_CFG = 0x000010U;
constexpr uint32_t TX_POWER = 0x01000CU;
constexpr uint32_t CHAN_CTRL = 0x010014U;
constexpr uint32_t TX_FCTRL = 0x000024U;
constexpr uint32_t TX_FCTRL_HI = 0x000028U;
constexpr uint32_t DX_TIME = 0x00002CU;
constexpr uint32_t RX_FWTO = 0x000034U;
constexpr uint32_t SYS_STATUS = 0x000044U;
constexpr uint32_t RX_FINFO = 0x00004CU;
constexpr uint32_t RX_TIME_0 = 0x000064U;
constexpr uint32_t TX_TIME_LO = 0x000074U;
constexpr uint32_t TX_ANTD = 0x010004U;
constexpr uint32_t CIA_CONF = 0x0E0000U;
constexpr uint32_t DGC_CFG = 0x030018U;
constexpr uint32_t DGC_CFG0 = 0x03001CU;
constexpr uint32_t DGC_CFG1 = 0x030020U;
constexpr uint32_t DGC_LUT_0 = 0x030038U;
constexpr uint32_t DGC_LUT_1 = 0x03003CU;
constexpr uint32_t DGC_LUT_2 = 0x030040U;
constexpr uint32_t DGC_LUT_3 = 0x030044U;
constexpr uint32_t DGC_LUT_4 = 0x030048U;
constexpr uint32_t DGC_LUT_5 = 0x03004CU;
constexpr uint32_t DGC_LUT_6 = 0x030050U;
constexpr uint32_t STS_CFG0 = 0x020000U;
constexpr uint32_t RX_CAL_CFG = 0x04000CU;
constexpr uint32_t RX_CAL_RESI = 0x040014U;
constexpr uint32_t RX_CAL_RESQ = 0x04001CU;
constexpr uint32_t RX_CAL_STS = 0x040020U;
constexpr uint32_t DTUNE0 = 0x060000U;
constexpr uint32_t PRE_TOC = 0x060004U;
constexpr uint32_t DTUNE3 = 0x06000CU;
constexpr uint32_t DTUNE4 = 0x060010U;
constexpr uint32_t RF_ENABLE = 0x070000U;
constexpr uint32_t RF_CTRL_MASK = 0x070004U;
constexpr uint32_t RF_SWITCH_CTRL = 0x070014U;
constexpr uint32_t RF_TX_CTRL_1 = 0x07001AU;
constexpr uint32_t RF_TX_CTRL_2 = 0x07001CU;
constexpr uint32_t LDO_CTRL = 0x070048U;
constexpr uint32_t LDO_RLOAD = 0x070050U;
constexpr uint32_t PLL_CFG = 0x090000U;
constexpr uint32_t PLL_CAL = 0x090008U;
constexpr uint32_t SYS_STATE = 0x0F0030U;
constexpr uint32_t CLK_CTRL = 0x110004U;
constexpr uint32_t SEQ_CTRL = 0x110008U;
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

constexpr size_t DW_TIME_LEN = 5U;
constexpr uint64_t DW_TIME_40_BIT_MASK = 0xFFFFFFFFFFULL;
constexpr uint64_t UUS_TO_DTU = 65536ULL;
constexpr uint32_t DX_TIME_BIT_MASK = 0xFFFFFFFEUL;
constexpr uint32_t RX_FWTO_BIT_MASK = 0x000FFFFFUL;

constexpr uint32_t SYS_CFG_PHR_MODE_BIT_OFFSET = 4U;
constexpr uint32_t SYS_CFG_PHR_MODE_BIT_MASK = 0x00000010UL;
constexpr uint32_t SYS_CFG_PHR_6M8_BIT_OFFSET = 5U;
constexpr uint32_t SYS_CFG_PHR_6M8_BIT_MASK = 0x00000020UL;
constexpr uint32_t SYS_CFG_CP_SPC_BIT_OFFSET = 12U;
constexpr uint32_t SYS_CFG_CP_SPC_BIT_MASK = 0x00003000UL;
constexpr uint32_t SYS_CFG_CP_SDC_BIT_OFFSET = 15U;
constexpr uint32_t SYS_CFG_CP_SDC_BIT_MASK = 0x00008000UL;
constexpr uint32_t SYS_CFG_PDOA_MODE_BIT_OFFSET = 16U;
constexpr uint32_t SYS_CFG_PDOA_MODE_BIT_MASK = 0x00030000UL;
constexpr uint32_t SYS_CFG_RXWTOE_BIT_MASK = 0x00000200UL;

constexpr uint32_t ACK_RESP = 0x010008U;
constexpr uint32_t ACK_RESP_W4R_TIM_BIT_MASK = 0x000FFFFFUL;

constexpr uint32_t CHAN_CTRL_RX_PCODE_BIT_OFFSET = 8U;
constexpr uint32_t CHAN_CTRL_RX_PCODE_BIT_MASK = 0x00001F00UL;
constexpr uint32_t CHAN_CTRL_TX_PCODE_BIT_OFFSET = 3U;
constexpr uint32_t CHAN_CTRL_TX_PCODE_BIT_MASK = 0x000000F8UL;
constexpr uint32_t CHAN_CTRL_SFD_TYPE_BIT_OFFSET = 1U;
constexpr uint32_t CHAN_CTRL_SFD_TYPE_BIT_MASK = 0x00000006UL;
constexpr uint32_t CHAN_CTRL_RF_CHAN_BIT_OFFSET = 0U;
constexpr uint32_t CHAN_CTRL_RF_CHAN_BIT_MASK = 0x00000001UL;

constexpr uint16_t DGC_CFG_RX_TUNE_EN_BIT_MASK = 0x0001U;
constexpr uint16_t DGC_CFG_THR_64_BIT_OFFSET = 9U;
constexpr uint16_t DGC_CFG_THR_64_BIT_MASK = 0x7E00U;
constexpr uint16_t DGC_CFG_THR_64_VALUE = 0x32U;
constexpr uint32_t DGC_CFG0_VALUE = 0x10000240UL;
constexpr uint32_t DGC_CFG1_VALUE = 0x1B6DA489UL;
constexpr uint32_t DGC_LUT_0_CH5 = 0x0001C0FDUL;
constexpr uint32_t DGC_LUT_1_CH5 = 0x0001C43EUL;
constexpr uint32_t DGC_LUT_2_CH5 = 0x0001C6BEUL;
constexpr uint32_t DGC_LUT_3_CH5 = 0x0001C77EUL;
constexpr uint32_t DGC_LUT_4_CH5 = 0x0001CF36UL;
constexpr uint32_t DGC_LUT_5_CH5 = 0x0001CFB5UL;
constexpr uint32_t DGC_LUT_6_CH5 = 0x0001CFF5UL;
constexpr uint32_t DGC_LUT_0_CH9 = 0x0002A8FEUL;
constexpr uint32_t DGC_LUT_1_CH9 = 0x0002AC36UL;
constexpr uint32_t DGC_LUT_2_CH9 = 0x0002A5FEUL;
constexpr uint32_t DGC_LUT_3_CH9 = 0x0002AF3EUL;
constexpr uint32_t DGC_LUT_4_CH9 = 0x0002AF7DUL;
constexpr uint32_t DGC_LUT_5_CH9 = 0x0002AFB5UL;
constexpr uint32_t DGC_LUT_6_CH9 = 0x0002AFB5UL;

constexpr uint8_t DTUNE0_PRE_PAC_SYM_BIT_OFFSET = 0U;
constexpr uint8_t DTUNE0_PRE_PAC_SYM_BIT_MASK = 0x03U;
constexpr uint8_t DTUNE0_DT0B4_BIT_MASK = 0x10U;
constexpr uint32_t DTUNE3_PD_THRESH_DEFAULT = 0xAF5F584CUL;
constexpr uint8_t STS_CFG0_STS_LEN_64 = 7U;
constexpr uint16_t DTUNE4_RX_SFD_HLDOFF_OFFSET = 3U;
constexpr uint8_t DTUNE4_RX_SFD_HLDOFF_DEF = 0x14U;
constexpr uint8_t DTUNE4_RX_SFD_HLDOFF_LONG = 0x20U;

constexpr uint32_t RF_TX_CTRL_2_CH5 = 0x1C071134UL;
constexpr uint32_t RF_TX_CTRL_2_CH9 = 0x1C010034UL;
constexpr uint32_t RF_SWITCH_AUTO = 0x1C000000UL;
constexpr uint32_t RF_SWITCH_TX = 0x01011100UL;
constexpr uint32_t RF_SWITCH_RX_CH5 = 0x1C010000UL;
constexpr uint32_t RF_SWITCH_RX_CH9 = 0x2A010000UL;
constexpr uint8_t RF_TX_CTRL_1_B2 = 0x0EU;
constexpr uint16_t PLL_CFG_CH5 = 0x1F3CU;
constexpr uint16_t PLL_CFG_CH9 = 0x0F3CU;
constexpr uint8_t PLL_CAL_CFG_LD = 0x81U;
constexpr uint8_t LDO_RLOAD_B1 = 0x14U;
constexpr uint16_t LDO_CTRL_RX_CAL_MASK = 0x0105U;
constexpr uint8_t RX_CAL_CFG_MODE_BIT_MASK = 0x03U;
constexpr uint8_t RX_CAL_CFG_MODE_CAL = 0x01U;
constexpr uint8_t RX_CAL_CFG_CAL_EN_BIT_MASK = 0x10U;
constexpr uint16_t RX_CAL_CFG_COMP_DLY_OFFSET = 2U;
constexpr uint16_t RX_CAL_CFG_COMP_DLY_VALUE = 0x0002U;
constexpr uint8_t RX_CAL_CFG_READ_EN_BIT_MASK = 0x01U;
constexpr uint8_t RX_CAL_STS_DONE_BIT_MASK = 0x01U;
constexpr uint32_t RX_CAL_RESULT_FAIL = 0x1FFFFFFFUL;
constexpr uint16_t CLK_CTRL_AUTO = 0x0200U;
constexpr uint8_t SEQ_CTRL_AUTO_INIT2IDLE_OFFSET = 1U;
constexpr uint8_t SEQ_CTRL_AUTO_INIT2IDLE_BIT_MASK = 0x01U;
constexpr int PLL_LOCK_MAX_RETRIES = 50;

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
constexpr uint32_t CMD_DTX = 0x3U;
constexpr uint32_t CMD_TX_W4R = 0xCU;
constexpr uint32_t CMD_DTX_W4R = 0xDU;
constexpr uint32_t SYS_STATUS_HPDWARN_BIT_MASK = 0x08000000UL;

}  // namespace dw3000::registers
