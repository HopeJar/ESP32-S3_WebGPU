/**
 * @file protocol.hpp
 * @brief Communication protocol middleware interface
 */

#pragma once

namespace Middleware {
namespace Protocol {

/**
 * @brief Initialize communication protocol
 * @return true if initialization successful, false otherwise
 */
bool initialize();

/**
 * @brief Process protocol events
 */
void process();

/**
 * @brief Deinitialize protocol
 */
void deinitialize();

} // namespace Protocol
} // namespace Middleware
