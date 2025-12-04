/**
 * @file http_server.hpp
 * @brief HTTP server interface
 */

#pragma once

namespace Web {
namespace HTTPServer {

/**
 * @brief Start HTTP server
 * @return true if server started successfully, false otherwise
 */
bool start();

/**
 * @brief Stop HTTP server
 */
void stop();

/**
 * @brief Check if server is running
 * @return true if running, false otherwise
 */
bool is_running();

} // namespace HTTPServer
} // namespace Web
