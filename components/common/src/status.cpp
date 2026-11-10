#include "status.hpp"

namespace feed {

const char* status_name(Status status) {
    switch (status) {
        case Status::Ok:
            return "ok";
        case Status::InvalidArgument:
            return "invalid_argument";
        case Status::NotImplemented:
            return "not_implemented";
        case Status::ProtocolUnverified:
            return "protocol_unverified";
        case Status::Denied:
            return "denied";
        case Status::Timeout:
            return "timeout";
        case Status::CrcMismatch:
            return "crc_mismatch";
        case Status::ShortFrame:
            return "short_frame";
        case Status::ProtocolError:
            return "protocol_error";
        case Status::NotCalibrated:
            return "not_calibrated";
        case Status::FaultLatched:
            return "fault_latched";
        case Status::StorageError:
            return "storage_error";
    }
    return "unknown";
}

}  // namespace feed
