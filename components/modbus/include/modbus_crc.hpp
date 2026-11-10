#pragma once

#include "status.hpp"

#include <cstddef>
#include <cstdint>

namespace feed {

uint16_t modbus_crc(const uint8_t* data, std::size_t length);

Status modbus_append_crc(uint8_t* frame, std::size_t length, std::size_t capacity, std::size_t& out_length);

Status modbus_check_crc(const uint8_t* frame, std::size_t length);

Status modbus_build_read_holding(uint8_t slave, uint16_t address, uint16_t quantity, uint8_t* out,
                                 std::size_t capacity, std::size_t& out_length);

bool modbus_crc_self_test();

}  // namespace feed
