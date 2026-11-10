#pragma once

#include <cstddef>
#include <cstdint>

namespace feed::hw {

// ESP32-S3-DevKitC-1 assignments.
// Forbidden on this board: strapping GPIO0, GPIO3, GPIO45, GPIO46;
// USB GPIO19 and GPIO20; console UART0 GPIO43 and GPIO44;
// flash/PSRAM GPIO26-GPIO37; RGB LED GPIO38 (v1.1) or GPIO48 (v1.0).

inline constexpr int kGpioFeederPwm = 4;
inline constexpr int kGpioServo1Pwm = 5;
inline constexpr int kGpioServo2Pwm = 6;
inline constexpr int kGpioMotorEnable = 7;
inline constexpr int kGpioServoBufferOe = 8;
inline constexpr int kGpioFeederBufferOe = 9;
inline constexpr int kGpioIoTx = 15;
inline constexpr int kGpioIoRx = 16;
inline constexpr int kGpioWeightTx = 17;
inline constexpr int kGpioWeightRx = 18;

// Not driven. Reserved if an automatic-direction converter fails bench testing.
inline constexpr int kReservedWeightDeGpio = 10;
inline constexpr int kReservedIoDeGpio = 11;

inline constexpr int kUartWeight = 1;
inline constexpr int kUartIo = 2;

inline constexpr int kLedcServoTimer = 0;
inline constexpr int kLedcFeederTimer = 1;
inline constexpr int kLedcServo1Channel = 0;
inline constexpr int kLedcServo2Channel = 1;
inline constexpr int kLedcFeederChannel = 2;

inline constexpr int kServoPwmHz = 50;
inline constexpr int kFeederPwmHz = 1000;
inline constexpr int kServoPeriodUs = 1000000 / kServoPwmHz;
inline constexpr int kServoDutyBits = 14;
inline constexpr int kFeederDutyBits = 10;
inline constexpr int kServoDutyMax = (1 << kServoDutyBits) - 1;
inline constexpr int kFeederDutyMax = (1 << kFeederDutyBits) - 1;

inline constexpr int kServoCenterUs = 1500;
inline constexpr int kAdvertisedPulseMinUs = 500;
inline constexpr int kAdvertisedPulseMaxUs = 2500;
inline constexpr int kProvisionalSafeMinUs = 1000;
inline constexpr int kProvisionalSafeMaxUs = 2000;

// These gates are firmware constants on purpose. A web request cannot open them.
inline constexpr bool kPhase1MotionEnabled = false;
inline constexpr bool kSafetyFeedbackPolarityVerified = false;
inline constexpr bool kQlMt01RegisterMapVerified = false;
inline constexpr bool kN4d8b08RegisterMapVerified = false;
inline constexpr bool kModbusTransmissionEnabled = false;

inline constexpr uint32_t kIoStaleMs = 200;
inline constexpr uint32_t kControlPeriodMs = 20;
inline constexpr char kApAddress[] = "192.168.4.1";

constexpr bool pin_allowed(int pin) {
    if (pin < 1 || pin > 48) {
        return false;
    }
    if (pin == 0 || pin == 3 || pin == 19 || pin == 20 || pin == 38 || pin == 48) {
        return false;
    }
    if (pin == 43 || pin == 44 || pin == 45 || pin == 46) {
        return false;
    }
    if (pin >= 26 && pin <= 37) {
        return false;
    }
    return true;
}

constexpr bool used_pins_valid() {
    constexpr int used[] = {
        kGpioFeederPwm,     kGpioServo1Pwm, kGpioServo2Pwm,    kGpioMotorEnable,
        kGpioServoBufferOe, kGpioFeederBufferOe, kGpioIoTx,    kGpioIoRx,
        kGpioWeightTx,      kGpioWeightRx,
    };
    constexpr int reserved[] = {kReservedWeightDeGpio, kReservedIoDeGpio};
    constexpr int used_count = static_cast<int>(sizeof(used) / sizeof(used[0]));
    constexpr int reserved_count = static_cast<int>(sizeof(reserved) / sizeof(reserved[0]));
    for (int i = 0; i < used_count; ++i) {
        if (!pin_allowed(used[i])) {
            return false;
        }
        for (int j = i + 1; j < used_count; ++j) {
            if (used[i] == used[j]) {
                return false;
            }
        }
        for (int r = 0; r < reserved_count; ++r) {
            if (used[i] == reserved[r] || !pin_allowed(reserved[r])) {
                return false;
            }
        }
    }
    return kUartWeight != 0 && kUartIo != 0 && kUartWeight != kUartIo;
}

inline uint32_t servo_duty_from_pulse_us(int pulse_us) {
    if (pulse_us <= 0) {
        return 0;
    }
    if (pulse_us >= kServoPeriodUs) {
        return static_cast<uint32_t>(kServoDutyMax);
    }
    return static_cast<uint32_t>(
        (static_cast<int64_t>(pulse_us) * (kServoDutyMax + 1)) / kServoPeriodUs);
}

inline uint32_t feeder_duty_from_percent(int percent) {
    if (percent <= 0) {
        return 0;
    }
    if (percent >= 100) {
        return static_cast<uint32_t>(kFeederDutyMax);
    }
    return static_cast<uint32_t>((static_cast<int64_t>(percent) * kFeederDutyMax) / 100);
}

inline bool pwm_math_self_test() {
    return servo_duty_from_pulse_us(0) == 0 && servo_duty_from_pulse_us(1500) == 1228 &&
           servo_duty_from_pulse_us(20000) == static_cast<uint32_t>(kServoDutyMax) &&
           feeder_duty_from_percent(0) == 0 && feeder_duty_from_percent(100) == 1023 &&
           feeder_duty_from_percent(30) == 306;
}

static_assert(used_pins_valid(), "GPIO map uses a forbidden, reserved, or duplicate pin");
static_assert(kServoPwmHz == 50, "Servo PWM frequency left the commissioning baseline");
static_assert(kFeederPwmHz == 1000, "Feeder PWM frequency left the commissioning baseline");

void boot_safe_outputs();

}  // namespace feed::hw
