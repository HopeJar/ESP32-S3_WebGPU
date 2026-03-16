/**
 * @file http_server.cpp
 * @brief HTTP server implementation
 */

#include "http_server.hpp"
#include "https_server.hpp"
#include "drivers/led_driver.hpp"
#include "esp_err.h"
#include "esp_http_server.h"
#include "logging/logger.hpp"
#include <cstddef>
#include <cstdio>

namespace Web {
namespace HTTPServer {

namespace {
    const Logging::ModuleLogger kLog(Logging::Module::HTTPServer);
    bool running = false;
    httpd_handle_t server = nullptr;
    bool https_enabled = false;

    // Embedded static web assets (linked into the binary by CMake)
    // - index_html_*: main HTML page
    // - style_css_*: stylesheet
    // - control_js_*: client-side JS for control UI
    extern const uint8_t index_html_start[] asm("_binary_index_html_start");
    extern const uint8_t index_html_end[] asm("_binary_index_html_end");
    extern const uint8_t style_css_start[] asm("_binary_style_css_start");
    extern const uint8_t style_css_end[] asm("_binary_style_css_end");
    extern const uint8_t control_js_start[] asm("_binary_control_js_start");
    extern const uint8_t control_js_end[] asm("_binary_control_js_end");
} // anonymous namespace

// Helper: send embedded static asset
// - Purpose: send a range of bytes (from embedded binary blobs) as an HTTP response
// - Parameters:
//   - `req`: HTTP request handle
//   - `start`, `end`: pointers to the start and end of the embedded data
//   - `type`: MIME type string (e.g. "text/html")
// - Returns: `ESP_OK` or an `esp_err_t` from `httpd_resp_send`
static esp_err_t send_embedded(httpd_req_t* req, const uint8_t* start, const uint8_t* end, const char* type) {
    httpd_resp_set_type(req, type);
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");

    size_t len = static_cast<size_t>(end - start);
    if (len > 0 && start[len - 1] == 0) {
        len -= 1;
    }

    return httpd_resp_send(req, reinterpret_cast<const char*>(start), len);
}

// Web API: GET /api/color
// - Purpose: return current LED color as JSON (r,g,b and hex string)
// - Method: GET
// - Example response: {"r":255,"g":128,"b":64,"hex":"#FF8040"}
static esp_err_t color_get_handler(httpd_req_t* req) {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    Drivers::LED::get_color(&r, &g, &b);

    char json[96];
    int len = std::snprintf(json, sizeof(json),
                            "{\"r\":%u,\"g\":%u,\"b\":%u,\"hex\":\"#%02X%02X%02X\"}",
                            r, g, b, r, g, b);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    return httpd_resp_send(req, json, len);
}

// Web API: GET / and GET /index.html
// - Purpose: serve the main HTML page embedded in the firmware
// - Method: GET
static esp_err_t index_get_handler(httpd_req_t* req) {
    return send_embedded(req, index_html_start, index_html_end, "text/html");
}

// Web API: GET /style.css
// - Purpose: serve embedded CSS for the web UI
// - Method: GET
static esp_err_t style_get_handler(httpd_req_t* req) {
    return send_embedded(req, style_css_start, style_css_end, "text/css");
}

// Web API: GET /control.js
// - Purpose: serve embedded JavaScript used by the control UI
// - Method: GET
static esp_err_t control_get_handler(httpd_req_t* req) {
    return send_embedded(req, control_js_start, control_js_end, "application/javascript");
}

static esp_err_t favicon_get_handler(httpd_req_t* req) {
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, nullptr, 0);
}

// Register all URI handlers for the server (main page, CSS, JS, API endpoints)
static esp_err_t register_handlers(httpd_handle_t handle) {
    httpd_uri_t index_uri = {};
    index_uri.uri = "/";
    index_uri.method = HTTP_GET;
    index_uri.handler = index_get_handler;
    index_uri.user_ctx = nullptr;

    // Register handler for "/" URL
    esp_err_t err = httpd_register_uri_handler(handle, &index_uri);
    if (err != ESP_OK) {
        APP_LOGE(kLog, "Failed to register index handler: %s", esp_err_to_name(err));
        return err;
    }

    // Also register handler for "/index.html" URL (same content as "/")
    httpd_uri_t index_html_uri = index_uri;
    index_html_uri.uri = "/index.html";
    err = httpd_register_uri_handler(handle, &index_html_uri);
    if (err != ESP_OK) {
        APP_LOGE(kLog, "Failed to register index.html handler: %s", esp_err_to_name(err));
        return err;
    }

    httpd_uri_t style_uri = {};
    style_uri.uri = "/style.css";
    style_uri.method = HTTP_GET;
    style_uri.handler = style_get_handler;
    style_uri.user_ctx = nullptr;

    //Whats a uri handler? A function that 
    // gets called when a request comes in for a specific URL (URI) and http method
    err = httpd_register_uri_handler(handle, &style_uri);
    if (err != ESP_OK) {
        APP_LOGE(kLog, "Failed to register style handler: %s", esp_err_to_name(err));
        return err;
    }

    httpd_uri_t control_uri = {};
    control_uri.uri = "/control.js";
    control_uri.method = HTTP_GET;
    control_uri.handler = control_get_handler;
    control_uri.user_ctx = nullptr;

    err = httpd_register_uri_handler(handle, &control_uri);
    if (err != ESP_OK) {
        APP_LOGE(kLog, "Failed to register control handler: %s", esp_err_to_name(err));
        return err;
    }

    httpd_uri_t favicon_uri = {};
    favicon_uri.uri = "/favicon.ico";
    favicon_uri.method = HTTP_GET;
    favicon_uri.handler = favicon_get_handler;
    favicon_uri.user_ctx = nullptr;

    err = httpd_register_uri_handler(handle, &favicon_uri);
    if (err != ESP_OK) {
        APP_LOGE(kLog, "Failed to register favicon handler: %s", esp_err_to_name(err));
        return err;
    }

    httpd_uri_t color_uri = {};
    color_uri.uri = "/api/color";
    color_uri.method = HTTP_GET;
    color_uri.handler = color_get_handler;
    color_uri.user_ctx = nullptr;

    err = httpd_register_uri_handler(handle, &color_uri);
    if (err != ESP_OK) {
        APP_LOGE(kLog, "Failed to register color handler: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

//Start the server if not already running and register URI handlers.
bool start() {
    if (running) {
        return true;
    }

    if (HTTPSServer::start(&server)) {
        https_enabled = true;
        running = (register_handlers(server) == ESP_OK);
        if (running) {
            return true;
        }

        HTTPSServer::stop(server);
        server = nullptr;
        https_enabled = false;
        APP_LOGW(kLog, "Failed to register HTTPS handlers, falling back to HTTP");
    } else {
        APP_LOGW(kLog, "Falling back to HTTP");
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    if (httpd_start(&server, &config) == ESP_OK) {
        https_enabled = false;
        running = (register_handlers(server) == ESP_OK);
        APP_LOGI(kLog, "HTTP server started on port 80");
        return running;
    }

    return false;
}

// Stop the Http server if already running.
void stop() {
    if (!running) {
        return;
    }

    APP_LOGI(kLog, "Stopping HTTP server");
    if (server) {
        if (https_enabled) {
            HTTPSServer::stop(server);
        } else {
            httpd_stop(server);
        }
        server = nullptr;
    }
    https_enabled = false;
    running = false;
}

bool is_running() {
    return running;
}

} // namespace HTTPServer
} // namespace Web
