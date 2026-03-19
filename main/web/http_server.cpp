/**
 * @file http_server.cpp
 * @brief HTTP server implementation
 */

#include "http_server.hpp"
#include "https_server.hpp"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "logging/logger.hpp"

#include <cstddef>
#include <cstdio>

namespace Web {
namespace HTTPServer {

namespace {
    const Logging::ModuleLogger kLog(Logging::Module::HTTPServer);
    constexpr const char* kDoomPartitionLabel = "web_assets";
    constexpr const char* kDoomBasePath = "/doomfs";

    bool running = false;
    bool https_enabled = false;
    bool doom_assets_mounted = false;
    httpd_handle_t server = nullptr;

    extern const uint8_t index_html_start[] asm("_binary_index_html_start");
    extern const uint8_t index_html_end[] asm("_binary_index_html_end");
    extern const uint8_t style_css_start[] asm("_binary_style_css_start");
    extern const uint8_t style_css_end[] asm("_binary_style_css_end");
    extern const uint8_t control_js_start[] asm("_binary_control_js_start");
    extern const uint8_t control_js_end[] asm("_binary_control_js_end");
} // namespace

static void set_no_store_headers(httpd_req_t* req) {
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
}

static esp_err_t send_error(httpd_req_t* req, const char* status, const char* message) {
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "text/plain");
    set_no_store_headers(req);
    return httpd_resp_send(req, message, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t send_embedded(httpd_req_t* req, const uint8_t* start, const uint8_t* end, const char* type) {
    httpd_resp_set_type(req, type);
    set_no_store_headers(req);

    size_t len = static_cast<size_t>(end - start);
    if (len > 0 && start[len - 1] == 0) {
        len -= 1;
    }

    return httpd_resp_send(req, reinterpret_cast<const char*>(start), len);
}

static bool mount_doom_assets() {
    if (doom_assets_mounted) {
        return true;
    }

    esp_vfs_spiffs_conf_t conf = {};
    conf.base_path = kDoomBasePath;
    conf.partition_label = kDoomPartitionLabel;
    conf.max_files = 6;
    conf.format_if_mount_failed = false;

    const esp_err_t err = esp_vfs_spiffs_register(&conf);
    if (err != ESP_OK) {
        APP_LOGW(kLog, "Failed to mount SPIFFS partition '%s': %s", kDoomPartitionLabel, esp_err_to_name(err));
        return false;
    }

    size_t total = 0;
    size_t used = 0;
    if (esp_spiffs_info(kDoomPartitionLabel, &total, &used) == ESP_OK) {
        APP_LOGI(kLog, "Mounted Doom assets partition: used=%u total=%u", static_cast<unsigned>(used), static_cast<unsigned>(total));
    }

    doom_assets_mounted = true;
    return true;
}

static void unmount_doom_assets() {
    if (!doom_assets_mounted) {
        return;
    }

    esp_vfs_spiffs_unregister(kDoomPartitionLabel);
    doom_assets_mounted = false;
}

static esp_err_t send_spiffs_file(httpd_req_t* req, const char* relative_path, const char* type) {
    if (!doom_assets_mounted && !mount_doom_assets()) {
        return send_error(req, "503 Service Unavailable", "Doom assets unavailable");
    }

    char full_path[128];
    const int path_len = std::snprintf(full_path, sizeof(full_path), "%s/%s", kDoomBasePath, relative_path);
    if (path_len <= 0 || path_len >= static_cast<int>(sizeof(full_path))) {
        return send_error(req, "500 Internal Server Error", "Asset path too long");
    }

    std::FILE* file = std::fopen(full_path, "rb");
    if (file == nullptr) {
        APP_LOGW(kLog, "Missing Doom asset: %s", full_path);
        return send_error(req, "404 Not Found", "Asset not found");
    }

    httpd_resp_set_type(req, type);
    set_no_store_headers(req);

    char buffer[1024];
    while (true) {
        const size_t bytes_read = std::fread(buffer, 1, sizeof(buffer), file);
        if (bytes_read > 0) {
            if (httpd_resp_send_chunk(req, buffer, bytes_read) != ESP_OK) {
                std::fclose(file);
                httpd_resp_send_chunk(req, nullptr, 0);
                return ESP_FAIL;
            }
        }

        if (bytes_read < sizeof(buffer)) {
            break;
        }
    }

    std::fclose(file);
    return httpd_resp_send_chunk(req, nullptr, 0);
}

static esp_err_t index_get_handler(httpd_req_t* req) {
    return send_embedded(req, index_html_start, index_html_end, "text/html");
}

static esp_err_t style_get_handler(httpd_req_t* req) {
    return send_embedded(req, style_css_start, style_css_end, "text/css");
}

static esp_err_t control_get_handler(httpd_req_t* req) {
    return send_embedded(req, control_js_start, control_js_end, "application/javascript");
}

static esp_err_t doom_js_get_handler(httpd_req_t* req) {
    return send_spiffs_file(req, "doomgeneric.js", "application/javascript");
}

static esp_err_t doom_wasm_get_handler(httpd_req_t* req) {
    return send_spiffs_file(req, "doomgeneric.wasm", "application/wasm");
}

static esp_err_t doom_data_get_handler(httpd_req_t* req) {
    return send_spiffs_file(req, "doomgeneric.data", "application/octet-stream");
}

static esp_err_t favicon_get_handler(httpd_req_t* req) {
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_set_type(req, "image/x-icon");
    set_no_store_headers(req);
    return httpd_resp_send(req, nullptr, 0);
}

static esp_err_t register_handlers(httpd_handle_t handle) {
    httpd_uri_t index_uri = {};
    index_uri.uri = "/";
    index_uri.method = HTTP_GET;
    index_uri.handler = index_get_handler;

    esp_err_t err = httpd_register_uri_handler(handle, &index_uri);
    if (err != ESP_OK) {
        APP_LOGE(kLog, "Failed to register index handler: %s", esp_err_to_name(err));
        return err;
    }

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
    err = httpd_register_uri_handler(handle, &style_uri);
    if (err != ESP_OK) {
        APP_LOGE(kLog, "Failed to register style handler: %s", esp_err_to_name(err));
        return err;
    }

    httpd_uri_t control_uri = {};
    control_uri.uri = "/control.js";
    control_uri.method = HTTP_GET;
    control_uri.handler = control_get_handler;
    err = httpd_register_uri_handler(handle, &control_uri);
    if (err != ESP_OK) {
        APP_LOGE(kLog, "Failed to register control handler: %s", esp_err_to_name(err));
        return err;
    }

    httpd_uri_t doom_js_uri = {};
    doom_js_uri.uri = "/doom/doomgeneric.js";
    doom_js_uri.method = HTTP_GET;
    doom_js_uri.handler = doom_js_get_handler;
    err = httpd_register_uri_handler(handle, &doom_js_uri);
    if (err != ESP_OK) {
        APP_LOGE(kLog, "Failed to register doomgeneric.js handler: %s", esp_err_to_name(err));
        return err;
    }

    httpd_uri_t doom_wasm_uri = {};
    doom_wasm_uri.uri = "/doom/doomgeneric.wasm";
    doom_wasm_uri.method = HTTP_GET;
    doom_wasm_uri.handler = doom_wasm_get_handler;
    err = httpd_register_uri_handler(handle, &doom_wasm_uri);
    if (err != ESP_OK) {
        APP_LOGE(kLog, "Failed to register doomgeneric.wasm handler: %s", esp_err_to_name(err));
        return err;
    }

    httpd_uri_t doom_data_uri = {};
    doom_data_uri.uri = "/doom/doomgeneric.data";
    doom_data_uri.method = HTTP_GET;
    doom_data_uri.handler = doom_data_get_handler;
    err = httpd_register_uri_handler(handle, &doom_data_uri);
    if (err != ESP_OK) {
        APP_LOGE(kLog, "Failed to register doomgeneric.data handler: %s", esp_err_to_name(err));
        return err;
    }

    httpd_uri_t favicon_uri = {};
    favicon_uri.uri = "/favicon.ico";
    favicon_uri.method = HTTP_GET;
    favicon_uri.handler = favicon_get_handler;
    err = httpd_register_uri_handler(handle, &favicon_uri);
    if (err != ESP_OK) {
        APP_LOGE(kLog, "Failed to register favicon handler: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

bool start() {
    if (running) {
        return true;
    }

    mount_doom_assets();

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
    unmount_doom_assets();
}

bool is_running() {
    return running;
}

} // namespace HTTPServer
} // namespace Web
