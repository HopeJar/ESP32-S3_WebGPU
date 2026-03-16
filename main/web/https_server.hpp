/**
 * @file https_server.hpp
 * @brief HTTPS server interface
 */

#pragma once

#include "esp_http_server.h"

namespace Web {
namespace HTTPSServer {

/**
 * @brief Start HTTPS server if TLS support and certs are available.
 * @param[out] handle Server handle on success.
 * @return true if HTTPS server started, false otherwise.
 */
bool start(httpd_handle_t* handle);

/**
 * @brief Stop a running HTTPS server.
 * @param handle Active HTTPS server handle.
 */
void stop(httpd_handle_t handle);

} // namespace HTTPSServer
} // namespace Web
