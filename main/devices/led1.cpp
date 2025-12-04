/**
 * @file led1.cpp
 * @brief LED1 device driver implementation
 */

#include "led1.hpp"
#include "esp_log.h"
#include "driver/gpio.h"

static const char* TAG = "LED1_Device";
#define LED1_GPIO 2 // Example GPIO, update as needed

namespace Devices {
namespace LED1 {

namespace {
    bool initialized = false;
    bool current_state = false;
}

bool initialize() {
    if (initialized) {
        ESP_LOGW(TAG, "LED1 already initialized");
        return true;
    }
    gpio_reset_pin(static_cast<gpio_num_t>(LED1_GPIO));
    gpio_set_direction(static_cast<gpio_num_t>(LED1_GPIO), GPIO_MODE_OUTPUT);
    gpio_set_level(static_cast<gpio_num_t>(LED1_GPIO), 0);
    initialized = true;
    current_state = false;
    ESP_LOGI(TAG, "LED1 initialized");
    return true;
}

void set_state(bool state) {
    if (!initialized) return;
    current_state = state;
    gpio_set_level(static_cast<gpio_num_t>(LED1_GPIO), state ? 1 : 0);
}

void deinitialize() {
    if (!initialized) return;
    gpio_reset_pin(static_cast<gpio_num_t>(LED1_GPIO));
    initialized = false;
}

} // namespace LED1
} // namespace Devices
