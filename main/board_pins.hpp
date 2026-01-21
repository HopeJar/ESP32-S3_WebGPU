/**
 * @file board_pins.hpp
 * @brief Board-specific pin mappings
 */

#pragma once

#include "driver/gpio.h"

namespace BoardPins {

// ESP32-S3-DevKitC-1 family RGB LED (NeoPixel/SK6812)
constexpr gpio_num_t RGB_LED_GPIO = GPIO_NUM_48;

} // namespace BoardPins
