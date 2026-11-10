#pragma once

#include "status.hpp"

#include <cstddef>
#include <cstdint>

namespace feed {

inline constexpr uint16_t kMachineSchema = 1;
inline constexpr uint16_t kServoSchema = 1;
inline constexpr uint16_t kWeighSchema = 1;
inline constexpr uint16_t kCommsSchema = 1;
inline constexpr uint16_t kNetworkSchema = 1;

inline constexpr char kDefaultSsid[] = "FeedBagging";
inline constexpr char kDefaultWifiPassword[] = "Bagging-Setup-1";
inline constexpr char kDefaultWebPassword[] = "operator-setup";
inline constexpr char kWebUser[] = "operator";

struct MachineParameters {
    uint16_t schema_version;
    uint16_t flags;
    int32_t target_weight_g;
    int32_t lower_limit_g;
    int32_t upper_limit_g;
    uint8_t coarse_feeder_percent;
    uint8_t medium_feeder_percent;
    uint8_t fine_feeder_percent;
    uint8_t coarse_gate_percent;
    uint8_t medium_gate_percent;
    uint8_t fine_gate_percent;
    uint8_t reserved0;
    uint8_t reserved1;
    int32_t coarse_to_medium_g;
    int32_t medium_to_fine_g;
    int32_t fine_cutoff_g;
    int32_t overrun_compensation_g;
    uint32_t stable_weight_ms;
    uint32_t discharge_ms;
    int32_t empty_hopper_g;
    uint32_t servo_move_ms;
    uint32_t bag_presence_debounce_ms;
    uint32_t bag_removal_timeout_ms;
};

struct ServoAxisCalibration {
    uint16_t closed_pulse_us;
    uint16_t open_pulse_us;
    uint16_t min_safe_pulse_us;
    uint16_t max_safe_pulse_us;
    uint8_t calibrated;
    uint8_t reserved0;
    uint16_t reserved1;
};

struct ServoCalibration {
    uint16_t schema_version;
    uint16_t reserved;
    ServoAxisCalibration feeder_gate;
    ServoAxisCalibration discharge_gate;
};

struct WeighingCalibration {
    uint16_t schema_version;
    uint8_t calibrated;
    uint8_t reserved;
    int32_t zero_offset_counts;
    int32_t span_counts;
    int32_t span_grams;
};

struct BusParameters {
    uint32_t baud;
    uint8_t slave_address;
    uint8_t parity;
    uint8_t stop_bits;
    uint8_t reserved;
    uint32_t timeout_ms;
};

struct CommsParameters {
    uint16_t schema_version;
    uint16_t reserved;
    BusParameters weighing;
    BusParameters industrial_io;
};

struct NetworkParameters {
    uint16_t schema_version;
    uint8_t web_password_is_default;
    uint8_t reserved;
    char ssid[33];
    char wifi_password[65];
    char web_password[33];
};

constexpr std::size_t text_length(const char* text) {
    std::size_t length = 0;
    while (text[length] != '\0') {
        ++length;
    }
    return length;
}

constexpr void copy_literal(char* destination, std::size_t capacity, const char* source) {
    std::size_t index = 0;
    for (; index + 1 < capacity && source[index] != '\0'; ++index) {
        destination[index] = source[index];
    }
    for (; index < capacity; ++index) {
        destination[index] = '\0';
    }
}

constexpr MachineParameters default_machine() {
    MachineParameters value = {};
    value.schema_version = kMachineSchema;
    value.flags = 1;
    value.target_weight_g = 2000;
    value.lower_limit_g = 1990;
    value.upper_limit_g = 2010;
    value.coarse_feeder_percent = 100;
    value.medium_feeder_percent = 60;
    value.fine_feeder_percent = 30;
    value.coarse_gate_percent = 100;
    value.medium_gate_percent = 60;
    value.fine_gate_percent = 20;
    // The four thresholds below are commissioning placeholders, not measured feed behavior.
    value.coarse_to_medium_g = 1800;
    value.medium_to_fine_g = 1960;
    value.fine_cutoff_g = 1995;
    value.overrun_compensation_g = 0;
    value.stable_weight_ms = 500;
    value.discharge_ms = 2000;
    value.empty_hopper_g = 20;
    value.servo_move_ms = 500;
    value.bag_presence_debounce_ms = 50;
    value.bag_removal_timeout_ms = 30000;
    return value;
}

constexpr ServoAxisCalibration default_axis() {
    ServoAxisCalibration axis = {};
    axis.closed_pulse_us = 1500;
    axis.open_pulse_us = 1500;
    axis.min_safe_pulse_us = 1000;
    axis.max_safe_pulse_us = 2000;
    axis.calibrated = 0;
    return axis;
}

constexpr ServoCalibration default_servo() {
    ServoCalibration value = {};
    value.schema_version = kServoSchema;
    value.feeder_gate = default_axis();
    value.discharge_gate = default_axis();
    return value;
}

constexpr WeighingCalibration default_weighing() {
    WeighingCalibration value = {};
    value.schema_version = kWeighSchema;
    value.calibrated = 0;
    return value;
}

constexpr BusParameters default_bus(uint32_t baud) {
    BusParameters value = {};
    value.baud = baud;
    value.slave_address = 0;
    value.parity = 0;
    value.stop_bits = 1;
    value.timeout_ms = 100;
    return value;
}

constexpr CommsParameters default_comms() {
    CommsParameters value = {};
    value.schema_version = kCommsSchema;
    value.weighing = default_bus(9600);
    value.industrial_io = default_bus(9600);
    return value;
}

constexpr NetworkParameters default_network() {
    NetworkParameters value = {};
    value.schema_version = kNetworkSchema;
    value.web_password_is_default = 1;
    copy_literal(value.ssid, sizeof(value.ssid), kDefaultSsid);
    copy_literal(value.wifi_password, sizeof(value.wifi_password), kDefaultWifiPassword);
    copy_literal(value.web_password, sizeof(value.web_password), kDefaultWebPassword);
    return value;
}

constexpr bool percent_ok(uint8_t value) {
    return value <= 100;
}

constexpr Status validate_machine(const MachineParameters& value) {
    if (value.schema_version != kMachineSchema) {
        return Status::InvalidArgument;
    }
    if (value.target_weight_g < 500 || value.target_weight_g > 5000) {
        return Status::InvalidArgument;
    }
    if (!(value.lower_limit_g < value.target_weight_g && value.target_weight_g < value.upper_limit_g)) {
        return Status::InvalidArgument;
    }
    const int32_t band = value.upper_limit_g - value.lower_limit_g;
    if (band < 2 || band > 1000) {
        return Status::InvalidArgument;
    }
    if (!percent_ok(value.coarse_feeder_percent) || !percent_ok(value.medium_feeder_percent) ||
        !percent_ok(value.fine_feeder_percent) || !percent_ok(value.coarse_gate_percent) ||
        !percent_ok(value.medium_gate_percent) || !percent_ok(value.fine_gate_percent)) {
        return Status::InvalidArgument;
    }
    if (!(value.coarse_feeder_percent >= value.medium_feeder_percent &&
          value.medium_feeder_percent >= value.fine_feeder_percent)) {
        return Status::InvalidArgument;
    }
    if (!(value.coarse_gate_percent >= value.medium_gate_percent &&
          value.medium_gate_percent >= value.fine_gate_percent)) {
        return Status::InvalidArgument;
    }
    if (!(value.coarse_to_medium_g < value.medium_to_fine_g &&
          value.medium_to_fine_g < value.fine_cutoff_g && value.fine_cutoff_g <= value.target_weight_g)) {
        return Status::InvalidArgument;
    }
    if (value.overrun_compensation_g < -200 || value.overrun_compensation_g > 200) {
        return Status::InvalidArgument;
    }
    if (value.empty_hopper_g < 0 || value.empty_hopper_g >= value.lower_limit_g) {
        return Status::InvalidArgument;
    }
    if (value.stable_weight_ms < 100 || value.stable_weight_ms > 5000) {
        return Status::InvalidArgument;
    }
    if (value.discharge_ms < 200 || value.discharge_ms > 30000) {
        return Status::InvalidArgument;
    }
    if (value.servo_move_ms < 100 || value.servo_move_ms > 3000) {
        return Status::InvalidArgument;
    }
    if (value.bag_presence_debounce_ms < 10 || value.bag_presence_debounce_ms > 500) {
        return Status::InvalidArgument;
    }
    if (value.bag_removal_timeout_ms < 1000 || value.bag_removal_timeout_ms > 120000) {
        return Status::InvalidArgument;
    }
    return Status::Ok;
}

constexpr bool pulse_in_window(uint16_t pulse, uint16_t minimum, uint16_t maximum) {
    return pulse >= minimum && pulse <= maximum;
}

constexpr Status validate_axis(const ServoAxisCalibration& axis, bool allow_calibrated) {
    if (axis.min_safe_pulse_us < 500 || axis.max_safe_pulse_us > 2500 ||
        axis.min_safe_pulse_us >= axis.max_safe_pulse_us) {
        return Status::InvalidArgument;
    }
    if (!pulse_in_window(axis.closed_pulse_us, axis.min_safe_pulse_us, axis.max_safe_pulse_us) ||
        !pulse_in_window(axis.open_pulse_us, axis.min_safe_pulse_us, axis.max_safe_pulse_us)) {
        return Status::InvalidArgument;
    }
    if (axis.calibrated > 1) {
        return Status::InvalidArgument;
    }
    if (axis.calibrated != 0 && !allow_calibrated) {
        return Status::Denied;
    }
    return Status::Ok;
}

constexpr Status validate_servo(const ServoCalibration& value, bool allow_calibrated) {
    if (value.schema_version != kServoSchema) {
        return Status::InvalidArgument;
    }
    const Status feeder = validate_axis(value.feeder_gate, allow_calibrated);
    if (feeder != Status::Ok) {
        return feeder;
    }
    return validate_axis(value.discharge_gate, allow_calibrated);
}

constexpr Status validate_weighing(const WeighingCalibration& value) {
    if (value.schema_version != kWeighSchema || value.calibrated != 0) {
        return value.calibrated != 0 ? Status::Denied : Status::InvalidArgument;
    }
    if (value.span_counts < 0 || value.span_grams < 0) {
        return Status::InvalidArgument;
    }
    if (value.zero_offset_counts < -1000000 || value.zero_offset_counts > 1000000) {
        return Status::InvalidArgument;
    }
    return Status::Ok;
}

constexpr bool baud_allowed(uint32_t baud) {
    return baud == 9600 || baud == 19200 || baud == 38400;
}

constexpr Status validate_bus(const BusParameters& value) {
    if (!baud_allowed(value.baud) || value.slave_address > 247 || value.parity > 2 ||
        (value.stop_bits != 1 && value.stop_bits != 2) || value.timeout_ms < 20 || value.timeout_ms > 1000) {
        return Status::InvalidArgument;
    }
    return Status::Ok;
}

constexpr Status validate_comms(const CommsParameters& value) {
    if (value.schema_version != kCommsSchema) {
        return Status::InvalidArgument;
    }
    const Status weighing = validate_bus(value.weighing);
    if (weighing != Status::Ok) {
        return weighing;
    }
    return validate_bus(value.industrial_io);
}

constexpr Status validate_network(const NetworkParameters& value) {
    if (value.schema_version != kNetworkSchema) {
        return Status::InvalidArgument;
    }
    const std::size_t ssid_length = text_length(value.ssid);
    const std::size_t wifi_length = text_length(value.wifi_password);
    const std::size_t web_length = text_length(value.web_password);
    if (ssid_length < 1 || ssid_length > 32 || wifi_length < 8 || wifi_length > 63 || web_length < 8 ||
        web_length > 32) {
        return Status::InvalidArgument;
    }
    return Status::Ok;
}

static_assert(validate_machine(default_machine()) == Status::Ok, "Default machine settings are invalid");
static_assert(validate_servo(default_servo(), false) == Status::Ok, "Default servo calibration is invalid");
static_assert(validate_weighing(default_weighing()) == Status::Ok, "Default weighing calibration is invalid");
static_assert(validate_comms(default_comms()) == Status::Ok, "Default communication settings are invalid");
static_assert(validate_network(default_network()) == Status::Ok, "Default network settings are invalid");
static_assert(text_length(kDefaultWifiPassword) >= 8, "Factory Wi-Fi password is too short");
static_assert(text_length(kDefaultWebPassword) >= 8, "Factory web password is too short");

bool settings_validate_self_test();

}  // namespace feed
