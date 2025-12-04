/**
 * @file spi_driver.hpp
 * @brief SPI middleware driver interface
 */

#pragma once

namespace Middleware {
namespace SPI {

/**
 * @brief Initialize SPI hardware
 * @return true if initialization successful, false otherwise
 */
bool initialize();

/**
 * @brief Process SPI events
 */
void process();

/**
 * @brief Deinitialize SPI driver
 */
void deinitialize();

} // namespace SPI
} // namespace Middleware
