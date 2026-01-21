/**
 * @file monitor_task.cpp
 * @brief Monitor task operations
 */

#include "freertos/tasks/monitor_task.hpp"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace Monitor {

namespace {
    const char* TAG = "MonitorTaskOp";
} // namespace

void MonitorOperation() {
    ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
    vTaskDelay(pdMS_TO_TICKS(5000)); // 5 second interval
}

} // namespace Monitor
