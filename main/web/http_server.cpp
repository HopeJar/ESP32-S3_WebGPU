/**
 * @file http_server.cpp
 * @brief HTTP server implementation
 */

#include "http_server.hpp"
#include "esp_log.h"

static const char* TAG = "HTTP_Server";

namespace Web {
namespace HTTPServer {

namespace {
    bool running = false;
} // anonymous namespace

bool start() {
    if (running) {
        ESP_LOGW(TAG, "HTTP server already running");
        return true;
    }

    ESP_LOGI(TAG, "HTTP server started");
    // Start HTTP server
    
    running = true;
    return true;
}

void stop() {
    if (!running) {
        return;
    }

    ESP_LOGI(TAG, "Stopping HTTP server");
    running = false;
}

bool is_running() {
    return running;
}

} // namespace HTTPServer
} // namespace Web
