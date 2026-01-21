/**
 * @file tasks.cpp
 * @brief FreeRTOS task implementations
 */

#include "tasks.hpp"
#include "esp_log.h"
#include "../../drivers/led_driver.hpp"
#include "../../drivers/wifi_driver.hpp"
#include "../../drivers/ethernet_driver.hpp"
#include <cstdio>

static const char* TAG = "Tasks";

namespace Tasks {

void blink_task(void* pvParameters) {
    ESP_LOGI(TAG, "Blink task started");
    ESP_LOGI(TAG, "  Stack size: %lu bytes", (unsigned long)Config::BLINK_TASK_STACK_SIZE);
    ESP_LOGI(TAG, "  Priority: %d", Config::BLINK_TASK_PRIORITY);

    struct {
        const char* name;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } rgb_colors[] = {
        {"Red", 255, 0, 0},
        {"Green", 0, 255, 0},
        {"Blue", 0, 0, 255}
    };
    size_t color_idx = 0;

    while (true) {
        ESP_LOGI(TAG, "LEDs set to %s (R=%u G=%u B=%u)", rgb_colors[color_idx].name,
                 rgb_colors[color_idx].r, rgb_colors[color_idx].g, rgb_colors[color_idx].b);
        Drivers::LED::set_color(rgb_colors[color_idx].r, rgb_colors[color_idx].g, rgb_colors[color_idx].b);

        color_idx = (color_idx + 1) % (sizeof(rgb_colors) / sizeof(rgb_colors[0]));
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second delay
    }
}

void wifi_task(void* pvParameters) {
    ESP_LOGI(TAG, "WiFi task started");
    ESP_LOGI(TAG, "  Stack size: %lu bytes", (unsigned long)Config::WIFI_TASK_STACK_SIZE);
    ESP_LOGI(TAG, "  Priority: %d", Config::WIFI_TASK_PRIORITY);

    Drivers::WiFi::initialize();

    while (true) {
        // WiFi driver main loop
        Drivers::WiFi::process();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void ethernet_task(void* pvParameters) {
    ESP_LOGI(TAG, "Ethernet task started");
    ESP_LOGI(TAG, "  Stack size: %lu bytes", (unsigned long)Config::ETH_TASK_STACK_SIZE);
    ESP_LOGI(TAG, "  Priority: %d", Config::ETH_TASK_PRIORITY);

    Drivers::Ethernet::initialize();

    while (true) {
        // Ethernet driver main loop
        Drivers::Ethernet::process();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void monitor_task(void* pvParameters) {
    ESP_LOGI(TAG, "System monitor task started");
    ESP_LOGI(TAG, "  Stack size: %lu bytes", (unsigned long)Config::MONITOR_TASK_STACK_SIZE);
    ESP_LOGI(TAG, "  Priority: %d", Config::MONITOR_TASK_PRIORITY);

    while (true) {
        // Report free heap
        ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(5000)); // 5 second interval
    }
}

TaskHandles create_all_tasks() {
    TaskHandles handles = {};

    ESP_LOGI(TAG, "Creating application tasks...");

    // Create blink task
    BaseType_t result = xTaskCreate(
        blink_task,
        Config::BLINK_TASK_NAME,
        Config::BLINK_TASK_STACK_SIZE,
        nullptr,
        Config::BLINK_TASK_PRIORITY,
        &handles.blink_task
    );
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create blink task!");
    }

    // Create WiFi task
    result = xTaskCreate(
        wifi_task,
        Config::WIFI_TASK_NAME,
        Config::WIFI_TASK_STACK_SIZE,
        nullptr,
        Config::WIFI_TASK_PRIORITY,
        &handles.wifi_task
    );
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create WiFi task!");
    }

    // Create Ethernet task
    result = xTaskCreate(
        ethernet_task,
        Config::ETH_TASK_NAME,
        Config::ETH_TASK_STACK_SIZE,
        nullptr,
        Config::ETH_TASK_PRIORITY,
        &handles.ethernet_task
    );
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Ethernet task!");
    }

    // Create monitor task
    result = xTaskCreate(
        monitor_task,
        Config::MONITOR_TASK_NAME,
        Config::MONITOR_TASK_STACK_SIZE,
        nullptr,
        Config::MONITOR_TASK_PRIORITY,
        &handles.monitor_task
    );
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create monitor task!");
    }

    ESP_LOGI(TAG, "All tasks created successfully");
    return handles;
}

} // namespace Tasks
