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
    (void)config;

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

    s_info.state = State::Ready;
    UWB_LOGI(TAG, LOG_FLAG_INIT, "DW3000 init devid=0x%08lx", static_cast<unsigned long>(s_info.device_id));
    return true;
}

DeviceInfo get_device_info()
{
    return s_info;
}

}  // namespace dw3000
