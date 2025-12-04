/**
 * @file ethernet_driver.hpp
 * @brief Ethernet driver interface
 */

#pragma once

namespace Drivers {
namespace Ethernet {

/**
 * @brief Initialize the Ethernet driver
 * @return true if initialization successful, false otherwise
 */
bool initialize();

/**
 * @brief Process Ethernet driver events (call periodically)
 */
void process();

/**
 * @brief Cleanup and deinitialize the Ethernet driver
 */
void deinitialize();

} // namespace Ethernet
} // namespace Drivers
