#include "modbus_master.hpp"

#include "hardware_config.hpp"

#include <cstring>

namespace feed {

ModbusMaster::ModbusMaster(Rs485Transport& transport) : transport_(transport), timeout_ms_(100) {}

Status ModbusMaster::transact(const uint8_t* request, std::size_t request_length, uint8_t* response,
                              std::size_t response_capacity, std::size_t& response_length) {
    response_length = 0;
    if (!hw::kModbusTransmissionEnabled) {
        return Status::Denied;
    }
    return transact_enabled(request, request_length, response, response_capacity, response_length);
}

Status ModbusMaster::transact_enabled(const uint8_t* request, std::size_t request_length, uint8_t* response,
                                      std::size_t response_capacity, std::size_t& response_length) {
    response_length = 0;
    if (request == nullptr || response == nullptr || request_length < 2 || request_length > 250 ||
        response_capacity < 5) {
        return Status::InvalidArgument;
    }
    uint8_t frame[256];
    std::memcpy(frame, request, request_length);
    std::size_t frame_length = 0;
    const Status crc_status = modbus_append_crc(frame, request_length, sizeof(frame), frame_length);
    if (crc_status != Status::Ok) {
        return crc_status;
    }

    transport_.flush_input();
    const Status write_status = transport_.write(frame, frame_length);
    if (write_status != Status::Ok) {
        return write_status;
    }

    uint8_t received[256];
    std::size_t received_length = 0;
    const Status read_status = transport_.read(received, sizeof(received), received_length, timeout_ms_);
    if (read_status != Status::Ok) {
        return read_status;
    }
    if (received_length < 4 || received_length > response_capacity) {
        return Status::ShortFrame;
    }
    const Status check = modbus_check_crc(received, received_length);
    if (check != Status::Ok) {
        return check;
    }
    if (received[0] != request[0]) {
        return Status::ProtocolError;
    }
    if ((received[1] & 0x80U) != 0) {
        return Status::ProtocolError;
    }
    std::memcpy(response, received, received_length);
    response_length = received_length;
    return Status::Ok;
}

}  // namespace feed
