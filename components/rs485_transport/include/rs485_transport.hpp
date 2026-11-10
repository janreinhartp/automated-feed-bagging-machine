#pragma once

#include "status.hpp"

#include <cstddef>
#include <cstdint>

namespace feed {

struct Rs485Pins {
    int uart_port;
    int tx_gpio;
    int rx_gpio;
    uint32_t baud;
    uint32_t timeout_ms;
};

class Rs485Transport {
public:
    Status init(const Rs485Pins& pins);
    Status write(const uint8_t* data, std::size_t length);
    Status read(uint8_t* data, std::size_t capacity, std::size_t& out_length, uint32_t timeout_ms);
    void flush_input();
    uint32_t baud() const { return baud_; }
    bool installed() const { return installed_; }

private:
    int uart_port_ = -1;
    uint32_t baud_ = 0;
    bool installed_ = false;
};

}  // namespace feed
