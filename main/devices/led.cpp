/**
 * @file led.cpp
 * @brief LED device driver implementation
 */

#include "led.hpp"
#include "esp_log.h"

static const char* TAG = "LED_Device";

namespace Devices {
namespace LED {

namespace {
    bool initialized = false;
} // anonymous namespace

bool initialize() {
    if (initialized) {
        ESP_LOGW(TAG, "LED device already initialized");
        return true;
    }

    ESP_LOGI(TAG, "LED device driver started");
    // Initialize LED device
    
    initialized = true;
    return true;
}

void deinitialize() {
    if (!initialized) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing LED device");
    initialized = false;
}

} // namespace LED
} // namespace Devices
