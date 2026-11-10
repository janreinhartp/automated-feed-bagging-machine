#include "modbus_crc.hpp"

namespace feed {

uint16_t modbus_crc(const uint8_t* data, std::size_t length) {
    uint16_t crc = 0xFFFF;
    if (data == nullptr) {
        return crc;
    }
    for (std::size_t i = 0; i < length; ++i) {
        crc = static_cast<uint16_t>(crc ^ data[i]);
        for (int bit = 0; bit < 8; ++bit) {
            if ((crc & 0x0001U) != 0) {
                crc = static_cast<uint16_t>((crc >> 1) ^ 0xA001U);
            } else {
                crc = static_cast<uint16_t>(crc >> 1);
            }
        }
    }
    return crc;
}

Status modbus_append_crc(uint8_t* frame, std::size_t length, std::size_t capacity, std::size_t& out_length) {
    out_length = 0;
    if (frame == nullptr || length == 0 || length + 2 > capacity) {
        return Status::InvalidArgument;
    }
    const uint16_t crc = modbus_crc(frame, length);
    frame[length] = static_cast<uint8_t>(crc & 0xFFU);
    frame[length + 1] = static_cast<uint8_t>((crc >> 8) & 0xFFU);
    out_length = length + 2;
    return Status::Ok;
}

Status modbus_check_crc(const uint8_t* frame, std::size_t length) {
    if (frame == nullptr || length < 4) {
        return Status::ShortFrame;
    }
    const uint16_t calculated = modbus_crc(frame, length - 2);
    const uint16_t received = static_cast<uint16_t>(frame[length - 2] | (frame[length - 1] << 8));
    if (calculated != received) {
        return Status::CrcMismatch;
    }
    return Status::Ok;
}

Status modbus_build_read_holding(uint8_t slave, uint16_t address, uint16_t quantity, uint8_t* out,
                                 std::size_t capacity, std::size_t& out_length) {
    out_length = 0;
    if (out == nullptr || slave == 0 || slave > 247 || quantity == 0 || quantity > 125 || capacity < 8) {
        return Status::InvalidArgument;
    }
    out[0] = slave;
    out[1] = 0x03;
    out[2] = static_cast<uint8_t>((address >> 8) & 0xFFU);
    out[3] = static_cast<uint8_t>(address & 0xFFU);
    out[4] = static_cast<uint8_t>((quantity >> 8) & 0xFFU);
    out[5] = static_cast<uint8_t>(quantity & 0xFFU);
    return modbus_append_crc(out, 6, capacity, out_length);
}

bool modbus_crc_self_test() {
    const uint8_t known[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};
    if (modbus_crc(known, sizeof(known)) != 0xCDC5) {
        return false;
    }
    const uint8_t text[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    if (modbus_crc(text, sizeof(text)) != 0x4B37) {
        return false;
    }
    uint8_t frame[8] = {};
    std::size_t length = 0;
    if (modbus_build_read_holding(1, 0, 10, frame, sizeof(frame), length) != Status::Ok || length != 8) {
        return false;
    }
    if (frame[6] != 0xC5 || frame[7] != 0xCD || modbus_check_crc(frame, length) != Status::Ok) {
        return false;
    }
    frame[7] ^= 0xFFU;
    if (modbus_check_crc(frame, length) != Status::CrcMismatch) {
        return false;
    }
    if (modbus_build_read_holding(0, 0, 1, frame, sizeof(frame), length) != Status::InvalidArgument) {
        return false;
    }
    return true;
}

}  // namespace feed
