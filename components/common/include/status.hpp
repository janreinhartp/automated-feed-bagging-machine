#pragma once

#include <cstddef>

namespace feed {

enum class Status : int {
    Ok = 0,
    InvalidArgument,
    NotImplemented,
    ProtocolUnverified,
    Denied,
    Timeout,
    CrcMismatch,
    ShortFrame,
    ProtocolError,
    NotCalibrated,
    FaultLatched,
    StorageError,
};

const char* status_name(Status status);

inline void copy_text(char* destination, std::size_t capacity, const char* source) {
    if (destination == nullptr || capacity == 0) {
        return;
    }
    if (source == nullptr) {
        destination[0] = '\0';
        return;
    }
    std::size_t index = 0;
    for (; index + 1 < capacity && source[index] != '\0'; ++index) {
        destination[index] = source[index];
    }
    destination[index] = '\0';
}

}  // namespace feed
