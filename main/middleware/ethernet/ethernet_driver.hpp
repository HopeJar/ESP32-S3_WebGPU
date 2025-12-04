/**
 * @file ethernet_driver.hpp
 * @brief Ethernet middleware driver interface
 */

#pragma once

namespace Middleware {
namespace Ethernet {

/**
 * @brief Initialize Ethernet hardware
 * @return true if initialization successful, false otherwise
 */
bool initialize();

/**
 * @brief Process Ethernet events
 */
void process();

/**
 * @brief Deinitialize Ethernet driver
 */
void deinitialize();

} // namespace Ethernet
} // namespace Middleware
