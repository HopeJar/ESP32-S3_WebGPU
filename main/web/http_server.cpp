/**
 * @file http_server.cpp
 * @brief HTTP server implementation
 */

#include "http_server.hpp"
#include "drivers/led_driver.hpp"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <cstddef>
#include <cstdio>

static const char* TAG = "HTTP_Server";

namespace Web {
namespace HTTPServer {

namespace {
    bool running = false;
    httpd_handle_t server = nullptr;

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

bool start() {
    if (running) {
        ESP_LOGW(TAG, "HTTP server already running");
        return true;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    esp_err_t err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(err));
        return false;
    }

    httpd_uri_t index_uri = {};
    index_uri.uri = "/";
    index_uri.method = HTTP_GET;
    index_uri.handler = index_get_handler;
    index_uri.user_ctx = nullptr;

    err = httpd_register_uri_handler(server, &index_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register index handler: %s", esp_err_to_name(err));
        httpd_stop(server);
        server = nullptr;
        return false;
    }

    httpd_uri_t index_html_uri = index_uri;
    index_html_uri.uri = "/index.html";
    err = httpd_register_uri_handler(server, &index_html_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register index.html handler: %s", esp_err_to_name(err));
        httpd_stop(server);
        server = nullptr;
        return false;
    }

    httpd_uri_t style_uri = {};
    style_uri.uri = "/style.css";
    style_uri.method = HTTP_GET;
    style_uri.handler = style_get_handler;
    style_uri.user_ctx = nullptr;

    err = httpd_register_uri_handler(server, &style_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register style handler: %s", esp_err_to_name(err));
        httpd_stop(server);
        server = nullptr;
        return false;
    }

    httpd_uri_t control_uri = {};
    control_uri.uri = "/control.js";
    control_uri.method = HTTP_GET;
    control_uri.handler = control_get_handler;
    control_uri.user_ctx = nullptr;

    err = httpd_register_uri_handler(server, &control_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register control handler: %s", esp_err_to_name(err));
        httpd_stop(server);
        server = nullptr;
        return false;
    }

    httpd_uri_t color_uri = {};
    color_uri.uri = "/api/color";
    color_uri.method = HTTP_GET;
    color_uri.handler = color_get_handler;
    color_uri.user_ctx = nullptr;

    err = httpd_register_uri_handler(server, &color_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register color handler: %s", esp_err_to_name(err));
        httpd_stop(server);
        server = nullptr;
        return false;
    }

    running = true;
    ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);
    return true;
}

void stop() {
    if (!running) {
        return;
    }

    ESP_LOGI(TAG, "Stopping HTTP server");
    if (server) {
        httpd_stop(server);
        server = nullptr;
    }
    running = false;
}

bool is_running() {
    return running;
}

} // namespace HTTPServer
} // namespace Web
