#include "settings.hpp"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include <cstring>

namespace feed {

namespace {
constexpr const char* kTag = "settings";

bool same_text(const char* left, const char* right) {
    if (left == nullptr || right == nullptr) {
        return false;
    }
    const std::size_t left_length = text_length(left);
    const std::size_t right_length = text_length(right);
    std::size_t index = 0;
    unsigned difference = static_cast<unsigned>(left_length ^ right_length);
    const std::size_t limit = left_length > right_length ? left_length : right_length;
    for (; index < limit; ++index) {
        const unsigned left_byte = index < left_length ? static_cast<unsigned char>(left[index]) : 0U;
        const unsigned right_byte = index < right_length ? static_cast<unsigned char>(right[index]) : 0U;
        difference |= left_byte ^ right_byte;
    }
    return difference == 0;
}

template <typename T>
Status load_blob(const char* nvs_namespace, T& value, T (*defaults)(), Status (*validate)(const T&)) {
    T loaded = defaults();
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(nvs_namespace, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        value = defaults();
        return Status::StorageError;
    }
    std::size_t size = sizeof(T);
    err = nvs_get_blob(handle, "blob", &loaded, &size);
    const bool stored_ok = err == ESP_OK && size == sizeof(T) && loaded.schema_version == defaults().schema_version &&
                           validate(loaded) == Status::Ok;
    if (!stored_ok) {
        loaded = defaults();
        err = nvs_set_blob(handle, "blob", &loaded, sizeof(T));
        if (err == ESP_OK) {
            err = nvs_commit(handle);
        }
        ESP_LOGW(kTag, "Replaced %s with safe defaults", nvs_namespace);
    }
    nvs_close(handle);
    value = loaded;
    return err == ESP_OK ? Status::Ok : Status::StorageError;
}

template <typename T>
Status store_blob(const char* nvs_namespace, const T& value) {
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(nvs_namespace, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return Status::StorageError;
    }
    err = nvs_set_blob(handle, "blob", &value, sizeof(value));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err == ESP_OK ? Status::Ok : Status::StorageError;
}

Status validate_servo_phase1(const ServoCalibration& value) {
    return validate_servo(value, false);
}

}  // namespace

Status SettingsStore::init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(kTag, "NVS partition was reset because its format could not be opened");
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    machine_ = default_machine();
    servo_ = default_servo();
    weighing_ = default_weighing();
    comms_ = default_comms();
    network_ = default_network();
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "NVS init failed; memory defaults remain in use");
        return Status::StorageError;
    }

    Status status = Status::Ok;
    if (load_blob<MachineParameters>("machine", machine_, default_machine, validate_machine) != Status::Ok) {
        status = Status::StorageError;
    }
    if (load_blob<ServoCalibration>("servo", servo_, default_servo, validate_servo_phase1) != Status::Ok) {
        status = Status::StorageError;
    }
    if (load_blob<WeighingCalibration>("weighcal", weighing_, default_weighing, validate_weighing) != Status::Ok) {
        status = Status::StorageError;
    }
    if (load_blob<CommsParameters>("comms", comms_, default_comms, validate_comms) != Status::Ok) {
        status = Status::StorageError;
    }
    if (load_blob<NetworkParameters>("network", network_, default_network, validate_network) != Status::Ok) {
        status = Status::StorageError;
    }
    return status;
}

Status SettingsStore::save_machine(const MachineParameters& value) {
    MachineParameters stored = value;
    stored.schema_version = kMachineSchema;
    stored.flags = 1;
    const Status valid = validate_machine(stored);
    if (valid != Status::Ok) {
        return valid;
    }
    const Status saved = store_blob("machine", stored);
    if (saved == Status::Ok) {
        machine_ = stored;
    }
    return saved;
}

Status SettingsStore::save_servo(const ServoCalibration& value) {
    ServoCalibration stored = value;
    stored.schema_version = kServoSchema;
    stored.feeder_gate.calibrated = 0;
    stored.discharge_gate.calibrated = 0;
    const Status valid = validate_servo(stored, false);
    if (valid != Status::Ok) {
        return valid;
    }
    const Status saved = store_blob("servo", stored);
    if (saved == Status::Ok) {
        servo_ = stored;
    }
    return saved;
}

Status SettingsStore::save_weighing(const WeighingCalibration& value) {
    WeighingCalibration stored = value;
    stored.schema_version = kWeighSchema;
    stored.calibrated = 0;
    const Status valid = validate_weighing(stored);
    if (valid != Status::Ok) {
        return valid;
    }
    const Status saved = store_blob("weighcal", stored);
    if (saved == Status::Ok) {
        weighing_ = stored;
    }
    return saved;
}

Status SettingsStore::save_comms(const CommsParameters& value) {
    CommsParameters stored = value;
    stored.schema_version = kCommsSchema;
    const Status valid = validate_comms(stored);
    if (valid != Status::Ok) {
        return valid;
    }
    const Status saved = store_blob("comms", stored);
    if (saved == Status::Ok) {
        comms_ = stored;
    }
    return saved;
}

Status SettingsStore::save_network(const NetworkParameters& value) {
    NetworkParameters stored = value;
    stored.schema_version = kNetworkSchema;
    const Status valid = validate_network(stored);
    if (valid != Status::Ok) {
        return valid;
    }
    const Status saved = store_blob("network", stored);
    if (saved == Status::Ok) {
        network_ = stored;
    }
    return saved;
}

Status SettingsStore::save_web_password(const char* next_password) {
    if (next_password == nullptr) {
        return Status::InvalidArgument;
    }
    NetworkParameters stored = network_;
    copy_literal(stored.web_password, sizeof(stored.web_password), next_password);
    stored.web_password_is_default = same_text(next_password, kDefaultWebPassword) ? 1 : 0;
    if (stored.web_password_is_default != 0) {
        return Status::InvalidArgument;
    }
    return save_network(stored);
}

bool SettingsStore::web_password_matches(const char* candidate) const {
    return same_text(candidate, network_.web_password);
}

}  // namespace feed
