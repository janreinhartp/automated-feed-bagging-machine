#pragma once

#include "modbus_crc.hpp"
#include "rs485_transport.hpp"

#include <cstddef>
#include <cstdint>

namespace feed {

class ModbusMaster {
public:
    explicit ModbusMaster(Rs485Transport& transport);

    // Generic Modbus RTU transaction. Phase 1 keeps transmission disabled, so
    // this returns Denied and does not write the UART.
    Status transact(const uint8_t* request, std::size_t request_length, uint8_t* response,
                    std::size_t response_capacity, std::size_t& response_length);

private:
    Status transact_enabled(const uint8_t* request, std::size_t request_length, uint8_t* response,
                            std::size_t response_capacity, std::size_t& response_length);

    Rs485Transport& transport_;
    uint32_t timeout_ms_;
};

}  // namespace feed
