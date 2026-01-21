/**
 * @file http_server.cpp
 * @brief HTTP server implementation
 */

#include "http_server.hpp"
#include "drivers/led_driver.hpp"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <cstdio>

static const char* TAG = "HTTP_Server";

namespace Web {
namespace HTTPServer {

namespace {
    bool running = false;
    httpd_handle_t server = nullptr;
} // anonymous namespace

esp_err_t color_get_handler(httpd_req_t* req) {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    Drivers::LED::get_color(&r, &g, &b);

    char json[96];
    int len = std::snprintf(json, sizeof(json),
                            "{\"r\":%u,\"g\":%u,\"b\":%u,\"hex\":\"#%02X%02X%02X\"}",
                            r, g, b, r, g, b);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, len);
}

esp_err_t root_get_handler(httpd_req_t* req) {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    Drivers::LED::get_color(&r, &g, &b);

    char html[768];
    int len = std::snprintf(
        html, sizeof(html),
        "<!DOCTYPE html><html><head><title>ESP32 Mastery</title>"
        "<style>"
        "body{margin:0;display:flex;flex-direction:column;align-items:center;justify-content:center;"
        "height:100vh;font-family:Verdana,Arial,sans-serif;background:rgb(%u,%u,%u);color:#fff;}"
        "h1{font-size:2.2rem;margin:0 0 0.5rem 0;}"
        "p{margin:0;opacity:0.9;}"
        "</style></head><body>"
        "<h1>Hello World</h1>"
        "<p id=\"color\">LED color: R=%u G=%u B=%u</p>"
        "<script>"
        "async function refreshColor(){"
        "const res=await fetch('/api/color');"
        "if(!res.ok) return;"
        "const c=await res.json();"
        "document.body.style.background='rgb('+c.r+','+c.g+','+c.b+')';"
        "document.getElementById('color').textContent='LED color: R='+c.r+' G='+c.g+' B='+c.b;"
        "}"
        "setInterval(refreshColor,500);"
        "refreshColor();"
        "</script>"
        "</body></html>",
        r, g, b, r, g, b);

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, len);
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

    httpd_uri_t root_uri = {};
    root_uri.uri = "/";
    root_uri.method = HTTP_GET;
    root_uri.handler = root_get_handler;
    root_uri.user_ctx = nullptr;

    err = httpd_register_uri_handler(server, &root_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register root handler: %s", esp_err_to_name(err));
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
