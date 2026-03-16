/**
 * @file logger.cpp
 * @brief Central logging configuration for application modules
 */

#include "logging/logger.hpp"

namespace Logging {

namespace {

struct ModuleConfig {
    const char* tag;
    esp_log_level_t level;
};

size_t to_index(Module module) {
    return static_cast<size_t>(module);
}

ModuleConfig g_module_configs[] = {
    {"BlinkTaskOp", ESP_LOG_WARN},
    {"HTTP_Server", ESP_LOG_INFO},
    {"HTTPS_Server", ESP_LOG_INFO},
};

} // namespace

void Logger::set_level(Module module, esp_log_level_t level) {
    ModuleConfig& config = g_module_configs[to_index(module)];
    config.level = level;
    esp_log_level_set(config.tag, level);
}

esp_log_level_t Logger::get_level(Module module) {
    return g_module_configs[to_index(module)].level;
}

const char* Logger::tag(Module module) {
    return g_module_configs[to_index(module)].tag;
}

void Logger::apply_levels() {
    for (const ModuleConfig& config : g_module_configs) {
        esp_log_level_set(config.tag, config.level);
    }
}

const char* ModuleLogger::tag() const {
    return Logger::tag(module_);
}

esp_log_level_t ModuleLogger::level() const {
    return Logger::get_level(module_);
}

void ModuleLogger::set_level(esp_log_level_t level) const {
    Logger::set_level(module_, level);
}

bool ModuleLogger::should_log(esp_log_level_t message_level) const {
    const esp_log_level_t configured_level = level();
    return configured_level != ESP_LOG_NONE && message_level <= configured_level;
}

} // namespace Logging
