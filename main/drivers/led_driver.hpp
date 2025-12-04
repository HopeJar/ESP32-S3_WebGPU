/**
 * @file led_driver.hpp
 * @brief LED driver interface for controlling onboard LED
 */

#pragma once

#include <cstdint>

namespace Drivers {
namespace LED {

/**
 * @brief Initialize the LED driver
 * @return true if initialization successful, false otherwise
 */
bool initialize();

/**
 * @brief Set the LED state
 * @param state true for ON, false for OFF
 */
void set_state(bool state);

/**
 * @brief Toggle the LED state
 */
void toggle();

/**
 * @brief Cleanup and deinitialize the LED driver
 */
void deinitialize();

} // namespace LED
} // namespace Drivers
