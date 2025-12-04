/**
 * @file wifi_driver.hpp
 * @brief WiFi middleware driver interface
 */

#pragma once

namespace Middleware {
namespace WiFi {

/**
 * @brief Initialize WiFi hardware
 * @return true if initialization successful, false otherwise
 */
bool initialize();

/**
 * @brief Process WiFi events
 */
void process();

/**
 * @brief Deinitialize WiFi driver
 */
void deinitialize();

} // namespace WiFi
} // namespace Middleware
