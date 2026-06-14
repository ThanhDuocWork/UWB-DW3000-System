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
bool read_reg(uint16_t reg, uint16_t subaddr, uint8_t *data, size_t size);
bool write_reg(uint16_t reg, uint16_t subaddr, const uint8_t *data, size_t size);
bool read_reg_u32(uint16_t reg, uint16_t subaddr, uint32_t *value);
uint32_t read_device_id();
bool transmit(const uint8_t *data, size_t size);
bool start_receive();

}  // namespace dw3000
