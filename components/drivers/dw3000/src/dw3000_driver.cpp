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

constexpr size_t kMaxHeaderSize = 3U;
constexpr uint8_t kRegIdMask = 0x3FU;
constexpr uint8_t kWriteFlag = 0x80U;
constexpr uint8_t kSubAddrFlag = 0x40U;
constexpr uint8_t kExtSubAddrFlag = 0x80U;
constexpr uint32_t kInvalidDeviceIdAllZero = 0x00000000UL;
constexpr uint32_t kInvalidDeviceIdAllOnes = 0xFFFFFFFFUL;
constexpr int kDeviceIdReadRetries = 3;

size_t build_header(bool is_write, uint16_t reg, uint16_t subaddr, uint8_t *header)
{
    if (header == nullptr || reg > kRegIdMask) {
        return 0U;
    }

    size_t header_len = 0U;
    header[header_len] = static_cast<uint8_t>(reg & kRegIdMask);
    if (is_write) {
        header[header_len] |= kWriteFlag;
    }

    if (subaddr != registers::NO_SUB_ADDRESS) {
        header[header_len] |= kSubAddrFlag;
        ++header_len;
        header[header_len] = static_cast<uint8_t>(subaddr & 0x7FU);
        if (subaddr > 0x7FU) {
            header[header_len] |= kExtSubAddrFlag;
            ++header_len;
            header[header_len] = static_cast<uint8_t>((subaddr >> 7) & 0xFFU);
        }
    }

    return header_len + 1U;
}

bool is_valid_device_id(uint32_t device_id)
{
    return device_id != kInvalidDeviceIdAllZero && device_id != kInvalidDeviceIdAllOnes;
}

}  // namespace

bool read_reg(uint16_t reg, uint16_t subaddr, uint8_t *data, size_t size)
{
    if (size > 0U && data == nullptr) {
        UWB_LOGE(TAG, "DW3000 read_reg null buffer reg=0x%02x", static_cast<unsigned>(reg));
        return false;
    }

    std::array<uint8_t, kMaxHeaderSize> header = {};
    const size_t header_len = build_header(false, reg, subaddr, header.data());
    if (header_len == 0U) {
        UWB_LOGE(TAG, "DW3000 read_reg invalid header reg=0x%02x sub=0x%04x",
                 static_cast<unsigned>(reg),
                 static_cast<unsigned>(subaddr));
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

    UWB_LOGD(TAG, LOG_FLAG_REG,
             "DW3000 read_reg reg=0x%02x sub=0x%04x len=%u",
             static_cast<unsigned>(reg),
             static_cast<unsigned>(subaddr),
             static_cast<unsigned>(size));
    return true;
}

bool write_reg(uint16_t reg, uint16_t subaddr, const uint8_t *data, size_t size)
{
    if (size > 0U && data == nullptr) {
        UWB_LOGE(TAG, "DW3000 write_reg null buffer reg=0x%02x", static_cast<unsigned>(reg));
        return false;
    }

    std::array<uint8_t, kMaxHeaderSize> header = {};
    const size_t header_len = build_header(true, reg, subaddr, header.data());
    if (header_len == 0U) {
        UWB_LOGE(TAG, "DW3000 write_reg invalid header reg=0x%02x sub=0x%04x",
                 static_cast<unsigned>(reg),
                 static_cast<unsigned>(subaddr));
        return false;
    }

    std::vector<uint8_t> tx(header_len + size, 0U);
    std::copy_n(header.data(), header_len, tx.data());
    if (size > 0U) {
        std::copy_n(data, size, tx.data() + header_len);
    }

    const bool ok = dw3000_hal::spi_write(tx.data(), tx.size());
    if (ok) {
        UWB_LOGD(TAG, LOG_FLAG_REG,
                 "DW3000 write_reg reg=0x%02x sub=0x%04x len=%u",
                 static_cast<unsigned>(reg),
                 static_cast<unsigned>(subaddr),
                 static_cast<unsigned>(size));
    }

    return ok;
}

bool read_reg_u32(uint16_t reg, uint16_t subaddr, uint32_t *value)
{
    if (value == nullptr) {
        UWB_LOGE(TAG, "DW3000 read_reg_u32 null value reg=0x%02x", static_cast<unsigned>(reg));
        return false;
    }

    uint8_t raw[registers::DEV_ID_LEN] = {};
    if (!read_reg(reg, subaddr, raw, sizeof(raw))) {
        return false;
    }

    *value =
        static_cast<uint32_t>(raw[0]) |
        (static_cast<uint32_t>(raw[1]) << 8U) |
        (static_cast<uint32_t>(raw[2]) << 16U) |
        (static_cast<uint32_t>(raw[3]) << 24U);
    return true;
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
        UWB_LOGE(TAG, "DW3000 init failed to read device id, raw=0x%08lx", static_cast<unsigned long>(s_info.device_id));
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
