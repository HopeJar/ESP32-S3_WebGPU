/**
 * @file led2.cpp
 * @brief LED2 device driver implementation
 */

#include "led2.hpp"
#include "esp_log.h"
#include "driver/gpio.h"

static const char* TAG = "LED2_Device";
#define LED2_GPIO 4 // Example GPIO, update as needed

namespace Devices {
namespace LED2 {

namespace {
    bool initialized = false;
    bool current_state = false;
}

bool initialize() {
    if (initialized) {
        ESP_LOGW(TAG, "LED2 already initialized");
        return true;
    }
    gpio_reset_pin(static_cast<gpio_num_t>(LED2_GPIO));
    gpio_set_direction(static_cast<gpio_num_t>(LED2_GPIO), GPIO_MODE_OUTPUT);
    gpio_set_level(static_cast<gpio_num_t>(LED2_GPIO), 0);
    initialized = true;
    current_state = false;
    ESP_LOGI(TAG, "LED2 initialized");
    return true;
}

void set_state(bool state) {
    if (!initialized) return;
    current_state = state;
    gpio_set_level(static_cast<gpio_num_t>(LED2_GPIO), state ? 1 : 0);
}

void deinitialize() {
    if (!initialized) return;
    gpio_reset_pin(static_cast<gpio_num_t>(LED2_GPIO));
    initialized = false;
}

} // namespace LED2
} // namespace Devices
