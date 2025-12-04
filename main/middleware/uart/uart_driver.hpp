/**
 * @file uart_driver.hpp
 * @brief UART middleware driver interface
 */

#pragma once

namespace Middleware {
namespace UART {

/**
 * @brief Initialize UART hardware
 * @return true if initialization successful, false otherwise
 */
bool initialize();

/**
 * @brief Process UART events
 */
void process();

/**
 * @brief Deinitialize UART driver
 */
void deinitialize();

} // namespace UART
} // namespace Middleware
