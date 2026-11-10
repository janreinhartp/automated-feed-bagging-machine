#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace feed {

// Holds the newest sample. A slow reader skips stale intermediate values.
template <typename T>
class Mailbox {
public:
    Mailbox() : mutex_(xSemaphoreCreateMutex()) {}

    ~Mailbox() {
        if (mutex_ != nullptr) {
            vSemaphoreDelete(mutex_);
        }
    }

    Mailbox(const Mailbox&) = delete;
    Mailbox& operator=(const Mailbox&) = delete;

    void publish(const T& value) {
        if (mutex_ == nullptr) {
            return;
        }
        if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(20)) != pdTRUE) {
            return;
        }
        value_ = value;
        published_ = true;
        xSemaphoreGive(mutex_);
    }

    bool read(T& out) const {
        if (mutex_ == nullptr || !published_) {
            return false;
        }
        if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(20)) != pdTRUE) {
            return false;
        }
        out = value_;
        xSemaphoreGive(mutex_);
        return true;
    }

private:
    mutable SemaphoreHandle_t mutex_;
    T value_{};
    bool published_ = false;
};

}  // namespace feed
