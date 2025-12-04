/**
 * @file led.hpp
 * @brief LED device driver interface
 */

#pragma once

namespace Devices {
namespace LED {

/**
 * @brief Initialize LED device
 * @return true if initialization successful, false otherwise
 */
bool initialize();

/**
 * @brief Deinitialize LED device
 */
void deinitialize();

} // namespace LED
} // namespace Devices
