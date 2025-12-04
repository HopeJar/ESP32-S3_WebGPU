/**
 * @file wifi_driver.hpp
 * @brief WiFi driver interface
 */

#pragma once

namespace Drivers {
namespace WiFi {

/**
 * @brief Initialize the WiFi driver
 * @return true if initialization successful, false otherwise
 */
bool initialize();

/**
 * @brief Process WiFi driver events (call periodically)
 */
void process();

/**
 * @brief Cleanup and deinitialize the WiFi driver
 */
void deinitialize();

} // namespace WiFi
} // namespace Drivers
