#include "dw3000_driver.h"

#include <algorithm>
#include <array>
#include <vector>

#include "dw3000_hal.h"
#include "dw3000_port.h"
#include "dw3000_registers.h"
#include "uwb_log.h"

namespace dw3000 {

static const char *TAG = "dw3000";
static DeviceInfo s_info{};

namespace {

constexpr size_t kMaxHeaderSize = 2U;
constexpr uint16_t kSpiWriteBit = 0x8000U;
constexpr uint16_t kSpiReadBit = 0x0000U;
constexpr uint8_t kSpiFastAccessCommand = 0x01U;
constexpr uint8_t kSpiFastAccessReadWrite = 0x00U;
constexpr uint8_t kSpiExtendedAddressReadWrite = 0x40U;
constexpr uint32_t kInvalidDeviceIdAllZero = 0x00000000UL;
constexpr uint32_t kInvalidDeviceIdAllOnes = 0xFFFFFFFFUL;
constexpr int kDeviceIdReadRetries = 3;

size_t build_header(bool is_write, uint32_t reg, uint16_t offset, size_t data_len, uint8_t *header)
{
    if (header == nullptr) {
        return 0U;
    }

    const uint16_t reg_file = static_cast<uint16_t>(0x1FU & ((reg + offset) >> 16U));
    const uint16_t reg_offset = static_cast<uint16_t>(0x7FU & (reg + offset));
    const uint16_t mode = is_write ? kSpiWriteBit : kSpiReadBit;
    const uint16_t addr = static_cast<uint16_t>((reg_file << 9U) | (reg_offset << 2U));

    if (is_write && data_len == 0U) {
        header[0] = static_cast<uint8_t>((kSpiWriteBit >> 8U) | ((reg & 0x1FU) << 1U) | kSpiFastAccessCommand);
        return 1U;
    }

    header[0] = static_cast<uint8_t>((mode | addr) >> 8U);
    header[1] = static_cast<uint8_t>(addr | (mode & 0x03U));

    if (reg_offset == 0U) {
        header[0] |= kSpiFastAccessReadWrite;
        return 1U;
    }

    header[0] |= kSpiExtendedAddressReadWrite;
    return 2U;
}

bool is_valid_device_id(uint32_t device_id)
{
    return device_id != kInvalidDeviceIdAllZero && device_id != kInvalidDeviceIdAllOnes;
}

bool is_valid_bringup_state(uint32_t sys_state)
{
    return sys_state != registers::DW_SYS_STATE_TXERR;
}

uint8_t encode_data_rate(int data_rate)
{
    if (data_rate == 6800) {
        return registers::DWT_BR_6M8;
    }

    return 0xFFU;
}

uint8_t encode_preamble_length(int preamble_length)
{
    if (preamble_length == 128) {
        return registers::DWT_PLEN_128;
    }

    return 0xFFU;
}

uint8_t encode_rx_pac(int rx_pac)
{
    switch (rx_pac) {
    case 8:
        return registers::DWT_PAC8;
    case 16:
        return registers::DWT_PAC16;
    case 32:
        return registers::DWT_PAC32;
    case 64:
        return registers::DWT_PAC64;
    default:
        return 0xFFU;
    }
}

bool write_reg_u16(uint32_t reg, uint16_t offset, uint16_t value)
{
    uint8_t raw[2] = {
        static_cast<uint8_t>(value & 0xFFU),
        static_cast<uint8_t>((value >> 8U) & 0xFFU),
    };
    return write_reg(reg, offset, raw, sizeof(raw));
}

bool write_reg_u8(uint32_t reg, uint16_t offset, uint8_t value)
{
    return write_reg(reg, offset, &value, sizeof(value));
}

bool read_reg_u8(uint32_t reg, uint16_t offset, uint8_t *value)
{
    if (value == nullptr) {
        return false;
    }

    return read_reg(reg, offset, value, sizeof(*value));
}

bool read_reg_u16(uint32_t reg, uint16_t offset, uint16_t *value)
{
    if (value == nullptr) {
        return false;
    }

    uint8_t raw[2] = {};
    if (!read_reg(reg, offset, raw, sizeof(raw))) {
        return false;
    }

    *value = static_cast<uint16_t>(raw[0]) |
             (static_cast<uint16_t>(raw[1]) << 8U);
    return true;
}

bool configure_radio(const Config &config)
{
    const uint8_t encoded_data_rate = encode_data_rate(config.data_rate);
    const uint8_t encoded_preamble_length = encode_preamble_length(config.preamble_length);
    const uint8_t encoded_rx_pac = encode_rx_pac(config.rx_pac);
    const uint16_t sfd_timeout = config.sfd_timeout == 0U ? registers::DWT_SFDTOC_DEF : config.sfd_timeout;

    if (config.channel != 5 && config.channel != 9) {
        UWB_LOGE(TAG, "DW3000 unsupported channel=%d", config.channel);
        return false;
    }

    if (encoded_data_rate == 0xFFU) {
        UWB_LOGE(TAG, "DW3000 unsupported data_rate=%d", config.data_rate);
        return false;
    }

    if (encoded_preamble_length == 0xFFU) {
        UWB_LOGE(TAG, "DW3000 unsupported preamble_length=%d", config.preamble_length);
        return false;
    }

    if (encoded_rx_pac == 0xFFU) {
        UWB_LOGE(TAG, "DW3000 unsupported rx_pac=%d", config.rx_pac);
        return false;
    }

    if (config.preamble_code < 1 || config.preamble_code > 31) {
        UWB_LOGE(TAG, "DW3000 invalid preamble_code=%d", config.preamble_code);
        return false;
    }

    if (config.sfd_type < 0 || config.sfd_type > 3) {
        UWB_LOGE(TAG, "DW3000 invalid sfd_type=%d", config.sfd_type);
        return false;
    }

    uint32_t chan_ctrl = 0U;
    if (!read_reg_u32(registers::CHAN_CTRL, registers::NO_SUB_ADDRESS, &chan_ctrl)) {
        UWB_LOGE(TAG, "DW3000 failed to read CHAN_CTRL");
        return false;
    }

    chan_ctrl &= ~(registers::CHAN_CTRL_RX_PCODE_BIT_MASK |
                   registers::CHAN_CTRL_TX_PCODE_BIT_MASK |
                   registers::CHAN_CTRL_SFD_TYPE_BIT_MASK |
                   registers::CHAN_CTRL_RF_CHAN_BIT_MASK);

    if (config.channel == 9) {
        chan_ctrl |= registers::CHAN_CTRL_RF_CHAN_BIT_MASK;
    }

    chan_ctrl |= registers::CHAN_CTRL_RX_PCODE_BIT_MASK &
                 (static_cast<uint32_t>(config.preamble_code) << registers::CHAN_CTRL_RX_PCODE_BIT_OFFSET);
    chan_ctrl |= registers::CHAN_CTRL_TX_PCODE_BIT_MASK &
                 (static_cast<uint32_t>(config.preamble_code) << registers::CHAN_CTRL_TX_PCODE_BIT_OFFSET);
    chan_ctrl |= registers::CHAN_CTRL_SFD_TYPE_BIT_MASK &
                 (static_cast<uint32_t>(config.sfd_type) << registers::CHAN_CTRL_SFD_TYPE_BIT_OFFSET);

    if (!write_reg_u32(registers::CHAN_CTRL, registers::NO_SUB_ADDRESS, chan_ctrl)) {
        UWB_LOGE(TAG, "DW3000 failed to write CHAN_CTRL");
        return false;
    }

    uint32_t tx_fctrl = 0U;
    if (!read_reg_u32(registers::TX_FCTRL, registers::NO_SUB_ADDRESS, &tx_fctrl)) {
        UWB_LOGE(TAG, "DW3000 failed to read TX_FCTRL");
        return false;
    }

    tx_fctrl &= ~(registers::TX_FCTRL_TXBR_BIT_MASK | registers::TX_FCTRL_TXPSR_BIT_MASK);
    tx_fctrl |= static_cast<uint32_t>(encoded_data_rate) << registers::TX_FCTRL_TXBR_BIT_OFFSET;
    tx_fctrl |= static_cast<uint32_t>(encoded_preamble_length) << registers::TX_FCTRL_TXPSR_BIT_OFFSET;

    if (!write_reg_u32(registers::TX_FCTRL, registers::NO_SUB_ADDRESS, tx_fctrl)) {
        UWB_LOGE(TAG, "DW3000 failed to write TX_FCTRL radio fields");
        return false;
    }

    uint8_t dtune0_cfg = 0U;
    if (!read_reg(registers::DTUNE0, 0U, &dtune0_cfg, sizeof(dtune0_cfg))) {
        UWB_LOGE(TAG, "DW3000 failed to read DTUNE0 pac field");
        return false;
    }

    dtune0_cfg &= static_cast<uint8_t>(~(registers::DTUNE0_PRE_PAC_SYM_BIT_MASK |
                                         registers::DTUNE0_DT0B4_BIT_MASK));
    dtune0_cfg |= encoded_rx_pac << registers::DTUNE0_PRE_PAC_SYM_BIT_OFFSET;

    if (!write_reg(registers::DTUNE0, 0U, &dtune0_cfg, sizeof(dtune0_cfg))) {
        UWB_LOGE(TAG, "DW3000 failed to write DTUNE0 pac field");
        return false;
    }

    if (!write_reg_u16(registers::DTUNE0, 2U, sfd_timeout)) {
        UWB_LOGE(TAG, "DW3000 failed to write SFD timeout=%u", static_cast<unsigned>(sfd_timeout));
        return false;
    }

    const uint8_t rx_sfd_holdoff =
        config.preamble_length > 64 ? registers::DTUNE4_RX_SFD_HLDOFF_LONG : registers::DTUNE4_RX_SFD_HLDOFF_DEF;
    if (!write_reg_u8(registers::DTUNE4, registers::DTUNE4_RX_SFD_HLDOFF_OFFSET, rx_sfd_holdoff)) {
        UWB_LOGE(TAG,
                 "DW3000 failed to write DTUNE4 RX_SFD_HLDOFF=0x%02x",
                 static_cast<unsigned>(rx_sfd_holdoff));
        return false;
    }

    uint8_t fine_preamble_length = 0U;
    if (!write_reg(registers::TX_FCTRL_HI, 1U, &fine_preamble_length, sizeof(fine_preamble_length))) {
        UWB_LOGE(TAG, "DW3000 failed to write TX_FCTRL_HI preamble fine length");
        return false;
    }

    UWB_LOGI(TAG,
             LOG_FLAG_INIT,
             "DW3000 config check channel=%d data_rate=%d preamble_length=%d preamble_code=%d rx_pac=%d sfd_type=%d sfd_timeout=%u sts_enabled=%d",
             config.channel,
             config.data_rate,
             config.preamble_length,
             config.preamble_code,
             config.rx_pac,
             config.sfd_type,
             static_cast<unsigned>(sfd_timeout),
             config.sts_enabled ? 1 : 0);

    uint32_t chan_ctrl_rb = 0U;
    uint32_t tx_fctrl_rb = 0U;
    uint16_t dtune0_rb = 0U;
    uint16_t sfd_timeout_rb = 0U;
    uint16_t pre_toc_rb = 0U;
    uint8_t rx_sfd_holdoff_rb = 0U;
    if (read_reg_u32(registers::CHAN_CTRL, registers::NO_SUB_ADDRESS, &chan_ctrl_rb) &&
        read_reg_u32(registers::TX_FCTRL, registers::NO_SUB_ADDRESS, &tx_fctrl_rb) &&
        read_reg_u16(registers::DTUNE0, 0U, &dtune0_rb) &&
        read_reg_u16(registers::DTUNE0, 2U, &sfd_timeout_rb) &&
        read_reg_u16(registers::PRE_TOC, registers::NO_SUB_ADDRESS, &pre_toc_rb) &&
        read_reg_u8(registers::DTUNE4, registers::DTUNE4_RX_SFD_HLDOFF_OFFSET, &rx_sfd_holdoff_rb)) {
        UWB_LOGD(TAG,
                 LOG_FLAG_DIAG,
                 "DW3000 radio readback CHAN_CTRL=0x%08lx TX_FCTRL=0x%08lx DTUNE0=0x%04x SFD_TO=%u PRE_TOC=%u DTUNE4_HLDOFF=0x%02x",
                 static_cast<unsigned long>(chan_ctrl_rb),
                 static_cast<unsigned long>(tx_fctrl_rb),
                 static_cast<unsigned>(dtune0_rb),
                 static_cast<unsigned>(sfd_timeout_rb),
                 static_cast<unsigned>(pre_toc_rb),
                 static_cast<unsigned>(rx_sfd_holdoff_rb));
    }
    return true;
}

bool configure_rx_tuning(const Config &config)
{
    const uint32_t dgc_lut[] = {
        config.channel == 9 ? registers::DGC_LUT_0_CH9 : registers::DGC_LUT_0_CH5,
        config.channel == 9 ? registers::DGC_LUT_1_CH9 : registers::DGC_LUT_1_CH5,
        config.channel == 9 ? registers::DGC_LUT_2_CH9 : registers::DGC_LUT_2_CH5,
        config.channel == 9 ? registers::DGC_LUT_3_CH9 : registers::DGC_LUT_3_CH5,
        config.channel == 9 ? registers::DGC_LUT_4_CH9 : registers::DGC_LUT_4_CH5,
        config.channel == 9 ? registers::DGC_LUT_5_CH9 : registers::DGC_LUT_5_CH5,
        config.channel == 9 ? registers::DGC_LUT_6_CH9 : registers::DGC_LUT_6_CH5,
    };
    const uint32_t dgc_lut_regs[] = {
        registers::DGC_LUT_0,
        registers::DGC_LUT_1,
        registers::DGC_LUT_2,
        registers::DGC_LUT_3,
        registers::DGC_LUT_4,
        registers::DGC_LUT_5,
        registers::DGC_LUT_6,
    };

    uint16_t dgc_cfg = 0U;
    if (!read_reg_u16(registers::DGC_CFG, registers::NO_SUB_ADDRESS, &dgc_cfg)) {
        UWB_LOGE(TAG, "DW3000 failed to read DGC_CFG");
        return false;
    }

    dgc_cfg |= registers::DGC_CFG_RX_TUNE_EN_BIT_MASK;
    if (!write_reg_u16(registers::DGC_CFG, registers::NO_SUB_ADDRESS, dgc_cfg)) {
        UWB_LOGE(TAG,
                 "DW3000 failed to enable DGC RX tuning cfg=0x%04x",
                 static_cast<unsigned>(dgc_cfg));
        return false;
    }

    if (!write_reg_u32(registers::DGC_CFG0, registers::NO_SUB_ADDRESS, registers::DGC_CFG0_VALUE) ||
        !write_reg_u32(registers::DGC_CFG1, registers::NO_SUB_ADDRESS, registers::DGC_CFG1_VALUE)) {
        UWB_LOGE(TAG, "DW3000 failed to write DGC_CFG0/DGC_CFG1");
        return false;
    }

    for (size_t i = 0; i < sizeof(dgc_lut_regs) / sizeof(dgc_lut_regs[0]); ++i) {
        if (!write_reg_u32(dgc_lut_regs[i], registers::NO_SUB_ADDRESS, dgc_lut[i])) {
            UWB_LOGE(TAG,
                     "DW3000 failed to write DGC_LUT_%u=0x%08lx",
                     static_cast<unsigned>(i),
                     static_cast<unsigned long>(dgc_lut[i]));
            return false;
        }
    }

    uint16_t dgc_cfg_rb = 0U;
    uint32_t dgc_lut0_rb = 0U;
    uint32_t dgc_lut6_rb = 0U;
    (void)read_reg_u16(registers::DGC_CFG, registers::NO_SUB_ADDRESS, &dgc_cfg_rb);
    (void)read_reg_u32(registers::DGC_LUT_0, registers::NO_SUB_ADDRESS, &dgc_lut0_rb);
    (void)read_reg_u32(registers::DGC_LUT_6, registers::NO_SUB_ADDRESS, &dgc_lut6_rb);

    UWB_LOGI(TAG,
             LOG_FLAG_INIT,
             "DW3000 RX tuning ready dgc_cfg=0x%04x lut0=0x%08lx lut6=0x%08lx",
             static_cast<unsigned>(dgc_cfg_rb),
             static_cast<unsigned long>(dgc_lut0_rb),
             static_cast<unsigned long>(dgc_lut6_rb));
    return true;
}

bool configure_rf_tuning(const Config &config)
{
    const uint32_t rf_tx_ctrl_2 =
        config.channel == 9 ? registers::RF_TX_CTRL_2_CH9 : registers::RF_TX_CTRL_2_CH5;
    const uint16_t pll_cfg =
        config.channel == 9 ? registers::PLL_CFG_CH9 : registers::PLL_CFG_CH5;

    if (!write_reg_u32(registers::TX_POWER, registers::NO_SUB_ADDRESS, config.tx_power)) {
        UWB_LOGE(TAG,
                 "DW3000 failed to write TX_POWER=0x%08lx",
                 static_cast<unsigned long>(config.tx_power));
        return false;
    }

    if (!write_reg_u32(registers::RF_TX_CTRL_2, registers::NO_SUB_ADDRESS, rf_tx_ctrl_2)) {
        UWB_LOGE(TAG,
                 "DW3000 failed to write RF_TX_CTRL_2=0x%08lx",
                 static_cast<unsigned long>(rf_tx_ctrl_2));
        return false;
    }

    if (!write_reg_u16(registers::PLL_CFG, registers::NO_SUB_ADDRESS, pll_cfg)) {
        UWB_LOGE(TAG,
                 "DW3000 failed to write PLL_CFG=0x%04x",
                 static_cast<unsigned>(pll_cfg));
        return false;
    }

    if (!write_reg_u8(registers::LDO_RLOAD, 1U, registers::LDO_RLOAD_B1)) {
        UWB_LOGE(TAG,
                 "DW3000 failed to write LDO_RLOAD[1]=0x%02x",
                 static_cast<unsigned>(registers::LDO_RLOAD_B1));
        return false;
    }

    if (!write_reg_u8(registers::RF_TX_CTRL_1, registers::NO_SUB_ADDRESS, registers::RF_TX_CTRL_1_B2)) {
        UWB_LOGE(TAG,
                 "DW3000 failed to write RF_TX_CTRL_1=0x%02x",
                 static_cast<unsigned>(registers::RF_TX_CTRL_1_B2));
        return false;
    }

    if (!write_reg_u8(registers::PLL_CAL, 0U, registers::PLL_CAL_CFG_LD)) {
        UWB_LOGE(TAG,
                 "DW3000 failed to write PLL_CAL=0x%02x",
                 static_cast<unsigned>(registers::PLL_CAL_CFG_LD));
        return false;
    }

    if (!write_reg_u32(registers::RF_SWITCH_CTRL, registers::NO_SUB_ADDRESS, registers::RF_SWITCH_AUTO)) {
        UWB_LOGE(TAG,
                 "DW3000 failed to write RF_SWITCH_AUTO=0x%08lx",
                 static_cast<unsigned long>(registers::RF_SWITCH_AUTO));
        return false;
    }

    uint32_t tx_power_rb = 0U;
    uint32_t rf_switch_rb = 0U;
    uint32_t rf_tx_ctrl_2_rb = 0U;
    uint16_t pll_cfg_rb = 0U;
    (void)read_reg_u32(registers::TX_POWER, registers::NO_SUB_ADDRESS, &tx_power_rb);
    (void)read_reg_u32(registers::RF_SWITCH_CTRL, registers::NO_SUB_ADDRESS, &rf_switch_rb);
    (void)read_reg_u32(registers::RF_TX_CTRL_2, registers::NO_SUB_ADDRESS, &rf_tx_ctrl_2_rb);
    (void)read_reg_u16(registers::PLL_CFG, registers::NO_SUB_ADDRESS, &pll_cfg_rb);

    UWB_LOGI(TAG,
             LOG_FLAG_INIT,
             "DW3000 RF tuning ready tx_power=0x%08lx rf_switch=0x%08lx rf_tx_ctrl_2=0x%08lx pll_cfg=0x%04x",
             static_cast<unsigned long>(tx_power_rb),
             static_cast<unsigned long>(rf_switch_rb),
             static_cast<unsigned long>(rf_tx_ctrl_2_rb),
             static_cast<unsigned>(pll_cfg_rb));
    return true;
}

bool lock_pll()
{
    (void)write_reg_u8(registers::SYS_STATUS,
                       registers::NO_SUB_ADDRESS,
                       static_cast<uint8_t>(registers::SYS_STATUS_CPLOCK_BIT_MASK));

    if (!write_reg_u16(registers::CLK_CTRL, registers::NO_SUB_ADDRESS, registers::CLK_CTRL_AUTO)) {
        UWB_LOGE(TAG, "DW3000 failed to set CLK_CTRL auto");
        return false;
    }

    uint8_t seq_ctrl_hi = 0U;
    if (!read_reg_u8(registers::SEQ_CTRL, registers::SEQ_CTRL_AUTO_INIT2IDLE_OFFSET, &seq_ctrl_hi)) {
        UWB_LOGE(TAG, "DW3000 failed to read SEQ_CTRL auto init field");
        return false;
    }

    seq_ctrl_hi |= registers::SEQ_CTRL_AUTO_INIT2IDLE_BIT_MASK;
    if (!write_reg_u8(registers::SEQ_CTRL, registers::SEQ_CTRL_AUTO_INIT2IDLE_OFFSET, seq_ctrl_hi)) {
        UWB_LOGE(TAG, "DW3000 failed to set SEQ_CTRL AUTO_INIT2IDLE");
        return false;
    }

    uint32_t sys_status = 0U;
    for (int attempt = 0; attempt < registers::PLL_LOCK_MAX_RETRIES; ++attempt) {
        dw3000_hal::delay_ms(1);
        if (!read_sys_status(&sys_status)) {
            return false;
        }

        if ((sys_status & registers::SYS_STATUS_CPLOCK_BIT_MASK) != 0U) {
            uint32_t sys_state = 0U;
            (void)read_sys_state(&sys_state);
            UWB_LOGI(TAG,
                     LOG_FLAG_INIT,
                     "DW3000 PLL locked SYS_STATUS=0x%08lx SYS_STATE=0x%08lx",
                     static_cast<unsigned long>(sys_status),
                     static_cast<unsigned long>(sys_state));
            return true;
        }
    }

    uint32_t sys_state = 0U;
    (void)read_sys_state(&sys_state);
    UWB_LOGE(TAG,
             "DW3000 PLL lock timeout SYS_STATUS=0x%08lx SYS_STATE=0x%08lx",
             static_cast<unsigned long>(sys_status),
             static_cast<unsigned long>(sys_state));
    return false;
}

bool run_rx_calibration()
{
    uint16_t ldo_ctrl = 0U;
    if (!read_reg_u16(registers::LDO_CTRL, registers::NO_SUB_ADDRESS, &ldo_ctrl)) {
        UWB_LOGE(TAG, "DW3000 RX cal failed to read LDO_CTRL");
        return false;
    }

    const uint16_t ldo_ctrl_cal = ldo_ctrl | registers::LDO_CTRL_RX_CAL_MASK;
    if (!write_reg_u16(registers::LDO_CTRL, registers::NO_SUB_ADDRESS, ldo_ctrl_cal)) {
        UWB_LOGE(TAG,
                 "DW3000 RX cal failed to enable LDO_CTRL=0x%04x",
                 static_cast<unsigned>(ldo_ctrl_cal));
        return false;
    }

    if (!write_reg_u16(registers::RX_CAL_CFG,
                       registers::RX_CAL_CFG_COMP_DLY_OFFSET,
                       registers::RX_CAL_CFG_COMP_DLY_VALUE)) {
        UWB_LOGE(TAG, "DW3000 RX cal failed to set COMP_DLY");
        (void)write_reg_u16(registers::LDO_CTRL, registers::NO_SUB_ADDRESS, ldo_ctrl);
        return false;
    }

    uint8_t cal_cfg = 0U;
    if (!read_reg_u8(registers::RX_CAL_CFG, registers::NO_SUB_ADDRESS, &cal_cfg)) {
        UWB_LOGE(TAG, "DW3000 RX cal failed to read RX_CAL_CFG");
        (void)write_reg_u16(registers::LDO_CTRL, registers::NO_SUB_ADDRESS, ldo_ctrl);
        return false;
    }

    cal_cfg &= static_cast<uint8_t>(~registers::RX_CAL_CFG_MODE_BIT_MASK);
    cal_cfg |= registers::RX_CAL_CFG_MODE_CAL;
    if (!write_reg_u8(registers::RX_CAL_CFG, registers::NO_SUB_ADDRESS, cal_cfg)) {
        UWB_LOGE(TAG, "DW3000 RX cal failed to set calibration mode");
        (void)write_reg_u16(registers::LDO_CTRL, registers::NO_SUB_ADDRESS, ldo_ctrl);
        return false;
    }

    (void)write_reg_u8(registers::RX_CAL_STS, registers::NO_SUB_ADDRESS, registers::RX_CAL_STS_DONE_BIT_MASK);

    cal_cfg |= registers::RX_CAL_CFG_CAL_EN_BIT_MASK;
    if (!write_reg_u8(registers::RX_CAL_CFG, registers::NO_SUB_ADDRESS, cal_cfg)) {
        UWB_LOGE(TAG, "DW3000 RX cal failed to start calibration");
        (void)write_reg_u16(registers::LDO_CTRL, registers::NO_SUB_ADDRESS, ldo_ctrl);
        return false;
    }

    dw3000_hal::delay_ms(1);

    uint8_t cal_sts = 0U;
    if (!read_reg_u8(registers::RX_CAL_STS, registers::NO_SUB_ADDRESS, &cal_sts)) {
        UWB_LOGE(TAG, "DW3000 RX cal failed to read RX_CAL_STS");
        (void)write_reg_u16(registers::LDO_CTRL, registers::NO_SUB_ADDRESS, ldo_ctrl);
        return false;
    }

    if ((cal_sts & registers::RX_CAL_STS_DONE_BIT_MASK) == 0U) {
        UWB_LOGE(TAG, "DW3000 RX cal timeout RX_CAL_STS=0x%02x", static_cast<unsigned>(cal_sts));
        (void)write_reg_u16(registers::LDO_CTRL, registers::NO_SUB_ADDRESS, ldo_ctrl);
        return false;
    }

    if (!write_reg_u8(registers::RX_CAL_CFG, registers::NO_SUB_ADDRESS, 0U)) {
        UWB_LOGE(TAG, "DW3000 RX cal failed to return normal mode");
        (void)write_reg_u16(registers::LDO_CTRL, registers::NO_SUB_ADDRESS, ldo_ctrl);
        return false;
    }

    (void)write_reg_u8(registers::RX_CAL_STS, registers::NO_SUB_ADDRESS, registers::RX_CAL_STS_DONE_BIT_MASK);

    uint8_t comp_dly_low = 0U;
    if (!read_reg_u8(registers::RX_CAL_CFG, registers::RX_CAL_CFG_COMP_DLY_OFFSET, &comp_dly_low)) {
        UWB_LOGE(TAG, "DW3000 RX cal failed to read result gate");
        (void)write_reg_u16(registers::LDO_CTRL, registers::NO_SUB_ADDRESS, ldo_ctrl);
        return false;
    }

    comp_dly_low |= registers::RX_CAL_CFG_READ_EN_BIT_MASK;
    if (!write_reg_u8(registers::RX_CAL_CFG, registers::RX_CAL_CFG_COMP_DLY_OFFSET, comp_dly_low)) {
        UWB_LOGE(TAG, "DW3000 RX cal failed to enable result read");
        (void)write_reg_u16(registers::LDO_CTRL, registers::NO_SUB_ADDRESS, ldo_ctrl);
        return false;
    }

    uint32_t resi = 0U;
    uint32_t resq = 0U;
    const bool read_results =
        read_reg_u32(registers::RX_CAL_RESI, registers::NO_SUB_ADDRESS, &resi) &&
        read_reg_u32(registers::RX_CAL_RESQ, registers::NO_SUB_ADDRESS, &resq);

    comp_dly_low &= static_cast<uint8_t>(~registers::RX_CAL_CFG_READ_EN_BIT_MASK);
    (void)write_reg_u8(registers::RX_CAL_CFG, registers::RX_CAL_CFG_COMP_DLY_OFFSET, comp_dly_low);
    (void)write_reg_u16(registers::LDO_CTRL, registers::NO_SUB_ADDRESS, ldo_ctrl);

    if (!read_results) {
        UWB_LOGE(TAG, "DW3000 RX cal failed to read calibration results");
        return false;
    }

    if (resi == registers::RX_CAL_RESULT_FAIL || resq == registers::RX_CAL_RESULT_FAIL) {
        UWB_LOGE(TAG,
                 "DW3000 RX cal invalid result resi=0x%08lx resq=0x%08lx",
                 static_cast<unsigned long>(resi),
                 static_cast<unsigned long>(resq));
        return false;
    }

    UWB_LOGI(TAG,
             LOG_FLAG_INIT,
             "DW3000 RX calibration ready sts=0x%02x resi=0x%08lx resq=0x%08lx",
             static_cast<unsigned>(cal_sts),
             static_cast<unsigned long>(resi),
             static_cast<unsigned long>(resq));
    return true;
}

}  // namespace

bool read_reg(uint32_t reg, uint16_t offset, uint8_t *data, size_t size)
{
    if (size > 0U && data == nullptr) {
        UWB_LOGE(TAG, "DW3000 read_reg null buffer reg=0x%06lx", static_cast<unsigned long>(reg));
        return false;
    }

    std::array<uint8_t, kMaxHeaderSize> header = {};
    const size_t header_len = build_header(false, reg, offset, size, header.data());
    if (header_len == 0U) {
        UWB_LOGE(TAG,
                 "DW3000 read_reg invalid header reg=0x%06lx off=0x%04x",
                 static_cast<unsigned long>(reg),
                 static_cast<unsigned>(offset));
        return false;
    }

    std::vector<uint8_t> tx(header_len + size, 0xFFU);
    std::copy_n(header.data(), header_len, tx.data());
    std::vector<uint8_t> rx(tx.size(), 0U);

    if (!dw3000_hal::spi_read(tx.data(), rx.data(), tx.size())) {
        return false;
    }

    if (size > 0U) {
        std::copy_n(rx.data() + header_len, size, data);
    }

    UWB_LOGD(TAG,
             LOG_FLAG_REG,
             "DW3000 read_reg reg=0x%06lx off=0x%04x len=%u",
             static_cast<unsigned long>(reg),
             static_cast<unsigned>(offset),
             static_cast<unsigned>(size));
    return true;
}

bool write_reg(uint32_t reg, uint16_t offset, const uint8_t *data, size_t size)
{
    if (size > 0U && data == nullptr) {
        UWB_LOGE(TAG, "DW3000 write_reg null buffer reg=0x%06lx", static_cast<unsigned long>(reg));
        return false;
    }

    std::array<uint8_t, kMaxHeaderSize> header = {};
    const size_t header_len = build_header(true, reg, offset, size, header.data());
    if (header_len == 0U) {
        UWB_LOGE(TAG,
                 "DW3000 write_reg invalid header reg=0x%06lx off=0x%04x",
                 static_cast<unsigned long>(reg),
                 static_cast<unsigned>(offset));
        return false;
    }

    std::vector<uint8_t> tx(header_len + size, 0U);
    std::copy_n(header.data(), header_len, tx.data());
    if (size > 0U) {
        std::copy_n(data, size, tx.data() + header_len);
    }

    const bool ok = dw3000_hal::spi_write(tx.data(), tx.size());
    if (ok) {
        UWB_LOGD(TAG,
                 LOG_FLAG_REG,
                 "DW3000 write_reg reg=0x%06lx off=0x%04x len=%u",
                 static_cast<unsigned long>(reg),
                 static_cast<unsigned>(offset),
                 static_cast<unsigned>(size));
    }

    return ok;
}

bool read_reg_u32(uint32_t reg, uint16_t offset, uint32_t *value)
{
    if (value == nullptr) {
        UWB_LOGE(TAG, "DW3000 read_reg_u32 null value reg=0x%06lx", static_cast<unsigned long>(reg));
        return false;
    }

    uint8_t raw[4] = {};
    if (!read_reg(reg, offset, raw, sizeof(raw))) {
        return false;
    }

    *value =
        static_cast<uint32_t>(raw[0]) |
        (static_cast<uint32_t>(raw[1]) << 8U) |
        (static_cast<uint32_t>(raw[2]) << 16U) |
        (static_cast<uint32_t>(raw[3]) << 24U);
    return true;
}

bool write_reg_u32(uint32_t reg, uint16_t offset, uint32_t value)
{
    uint8_t raw[4] = {
        static_cast<uint8_t>(value & 0xFFU),
        static_cast<uint8_t>((value >> 8U) & 0xFFU),
        static_cast<uint8_t>((value >> 16U) & 0xFFU),
        static_cast<uint8_t>((value >> 24U) & 0xFFU),
    };
    return write_reg(reg, offset, raw, sizeof(raw));
}

bool read_sys_status(uint32_t *status)
{
    return read_reg_u32(registers::SYS_STATUS, registers::NO_SUB_ADDRESS, status);
}

bool clear_sys_status(uint32_t mask)
{
    return write_reg_u32(registers::SYS_STATUS, 0U, mask);
}

bool read_sys_state(uint32_t *state)
{
    return read_reg_u32(registers::SYS_STATE, registers::NO_SUB_ADDRESS, state);
}

bool sys_status_is_ready_after_boot(uint32_t status)
{
    const uint32_t required_mask =
        registers::SYS_STATUS_RCINIT_BIT_MASK |
        registers::SYS_STATUS_SPIRDY_BIT_MASK;
    return (status & required_mask) == required_mask;
}

bool sys_status_has_tx_done(uint32_t status)
{
    return (status & registers::SYS_STATUS_TXFRS_BIT_MASK) != 0U;
}

bool sys_status_has_rx_good(uint32_t status)
{
    return (status & registers::SYS_STATUS_RXFCG_BIT_MASK) != 0U;
}

bool sys_status_has_rx_error(uint32_t status)
{
    return (status & registers::SYS_STATUS_ALL_RX_ERR) != 0U;
}

uint32_t sys_status_tx_done_mask()
{
    return registers::SYS_STATUS_TXFRS_BIT_MASK;
}

uint32_t sys_status_rx_good_mask()
{
    return registers::SYS_STATUS_RXFCG_BIT_MASK;
}

uint32_t sys_status_rx_error_mask()
{
    return registers::SYS_STATUS_ALL_RX_ERR;
}

bool issue_command(uint32_t command)
{
    const bool ok = write_reg(command, 0U, nullptr, 0U);
    if (ok) {
        UWB_LOGD(TAG, LOG_FLAG_REG, "DW3000 command=0x%02lx", static_cast<unsigned long>(command));
    }
    return ok;
}

uint32_t read_device_id()
{
    uint32_t device_id = 0U;
    if (!read_reg_u32(registers::DEV_ID, registers::NO_SUB_ADDRESS, &device_id)) {
        return 0U;
    }

    return device_id;
}

bool init(const Config &config)
{
    if (!port_init()) {
        s_info.state = State::Error;
        return false;
    }

    if (!dw3000_hal::reset_chip()) {
        s_info.state = State::Error;
        return false;
    }

    uint32_t device_id = 0U;
    for (int attempt = 0; attempt < kDeviceIdReadRetries; ++attempt) {
        device_id = read_device_id();
        if (is_valid_device_id(device_id)) {
            break;
        }
        dw3000_hal::delay_ms(5);
    }

    s_info.device_id = device_id;
    if (!is_valid_device_id(s_info.device_id)) {
        s_info.state = State::Error;
        UWB_LOGE(TAG,
                 "DW3000 init failed to read device id, raw=0x%08lx",
                 static_cast<unsigned long>(s_info.device_id));
        return false;
    }

    uint32_t sys_status = 0U;
    if (!read_sys_status(&sys_status)) {
        s_info.state = State::Error;
        UWB_LOGE(TAG, "DW3000 init failed to read SYS_STATUS after DEV_ID");
        return false;
    }

    UWB_LOGI(TAG,
             LOG_FLAG_INIT,
             "DW3000 bring-up SYS_STATUS=0x%08lx rcinit=%d spirdy=%d cplock=%d",
             static_cast<unsigned long>(sys_status),
             (sys_status & registers::SYS_STATUS_RCINIT_BIT_MASK) != 0U ? 1 : 0,
             (sys_status & registers::SYS_STATUS_SPIRDY_BIT_MASK) != 0U ? 1 : 0,
             (sys_status & registers::SYS_STATUS_CPLOCK_BIT_MASK) != 0U ? 1 : 0);

    if (!sys_status_is_ready_after_boot(sys_status)) {
        s_info.state = State::Error;
        UWB_LOGE(TAG,
                 "DW3000 init status not ready SYS_STATUS=0x%08lx",
                 static_cast<unsigned long>(sys_status));
        return false;
    }

    uint32_t sys_state = 0U;
    if (!read_sys_state(&sys_state)) {
        s_info.state = State::Error;
        UWB_LOGE(TAG, "DW3000 init failed to read SYS_STATE after SYS_STATUS");
        return false;
    }

    UWB_LOGI(TAG,
             LOG_FLAG_INIT,
             "DW3000 bring-up SYS_STATE=0x%08lx",
             static_cast<unsigned long>(sys_state));

    if (!is_valid_bringup_state(sys_state)) {
        s_info.state = State::Error;
        UWB_LOGE(TAG,
                 "DW3000 init invalid SYS_STATE=0x%08lx",
                 static_cast<unsigned long>(sys_state));
        return false;
    }

    if (!configure_radio(config)) {
        s_info.state = State::Error;
        UWB_LOGE(TAG, "DW3000 init failed to configure radio");
        return false;
    }

    if (!configure_rx_tuning(config)) {
        s_info.state = State::Error;
        UWB_LOGE(TAG, "DW3000 init failed to configure RX tuning");
        return false;
    }

    if (!configure_rf_tuning(config)) {
        s_info.state = State::Error;
        UWB_LOGE(TAG, "DW3000 init failed to configure RF tuning");
        return false;
    }

    if (!lock_pll()) {
        s_info.state = State::Error;
        UWB_LOGE(TAG, "DW3000 init failed to lock PLL");
        return false;
    }

    if (!run_rx_calibration()) {
        s_info.state = State::Error;
        UWB_LOGE(TAG, "DW3000 init failed to calibrate RX");
        return false;
    }

    s_info.state = State::Ready;
    UWB_LOGI(TAG, LOG_FLAG_INIT, "DW3000 init devid=0x%08lx", static_cast<unsigned long>(s_info.device_id));
    return true;
}

DeviceInfo get_device_info()
{
    return s_info;
}

}  // namespace dw3000
