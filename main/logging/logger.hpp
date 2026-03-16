/**
 * @file logger.hpp
 * @brief Application logging configuration and helpers
 */

#pragma once

#include "esp_log.h"

#include <cstddef>

namespace Logging {

enum class Module : size_t {
    BlinkTask,
    HTTPServer,
    HTTPSServer,
    Count
};

class ModuleLogger {
public:
    constexpr explicit ModuleLogger(Module module) : module_(module) {}

    const char* tag() const;
    esp_log_level_t level() const;
    void set_level(esp_log_level_t level) const;
    bool should_log(esp_log_level_t message_level) const;

private:
    Module module_;
};

class Logger {
public:
    static void set_level(Module module, esp_log_level_t level);
    static esp_log_level_t get_level(Module module);
    static const char* tag(Module module);
    static void apply_levels();
};

} // namespace Logging

#define APP_LOGE(logger, fmt, ...) \
    do { \
        if ((logger).should_log(ESP_LOG_ERROR)) { \
            ESP_LOGE((logger).tag(), fmt, ##__VA_ARGS__); \
        } \
    } while (0)

#define APP_LOGW(logger, fmt, ...) \
    do { \
        if ((logger).should_log(ESP_LOG_WARN)) { \
            ESP_LOGW((logger).tag(), fmt, ##__VA_ARGS__); \
        } \
    } while (0)

#define APP_LOGI(logger, fmt, ...) \
    do { \
        if ((logger).should_log(ESP_LOG_INFO)) { \
            ESP_LOGI((logger).tag(), fmt, ##__VA_ARGS__); \
        } \
    } while (0)

#define APP_LOGD(logger, fmt, ...) \
    do { \
        if ((logger).should_log(ESP_LOG_DEBUG)) { \
            ESP_LOGD((logger).tag(), fmt, ##__VA_ARGS__); \
        } \
    } while (0)

#define APP_LOGV(logger, fmt, ...) \
    do { \
        if ((logger).should_log(ESP_LOG_VERBOSE)) { \
            ESP_LOGV((logger).tag(), fmt, ##__VA_ARGS__); \
        } \
    } while (0)
