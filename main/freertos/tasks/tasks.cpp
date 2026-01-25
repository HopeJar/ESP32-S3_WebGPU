/**
 * @file tasks.cpp
 * @brief FreeRTOS task implementations
 */

#include "tasks.hpp"
#include "esp_log.h"
#include "drivers/led_driver.hpp"
#include "freertos/tasks/blink_task.hpp"
#include "freertos/tasks/wifi_task.hpp"
#include "freertos/tasks/ethernet_task.hpp"
#include "freertos/tasks/monitor_task.hpp"

static const char* MAIN_TAG = "Main";
static const char* TAG = "Tasks";

extern "C" void app_main(void)
{
    ESP_LOGI(MAIN_TAG, "========================================");
    ESP_LOGI(MAIN_TAG, "ESP32-S3 Application Starting");
    ESP_LOGI(MAIN_TAG, "========================================");

    // Initialize hardware drivers
    ESP_LOGI(MAIN_TAG, "Initializing drivers...");

    if (!Drivers::LED::initialize()) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize LED driver!");
    }

    ESP_LOGI(MAIN_TAG, "Driver initialization complete");

    // Create all FreeRTOS tasks
    ESP_LOGI(MAIN_TAG, "Creating tasks...");
    Tasks::TaskHandles handles = Tasks::create_all_tasks();
    (void)handles;

    ESP_LOGI(MAIN_TAG, "========================================");
    ESP_LOGI(MAIN_TAG, "Application initialization complete");
    ESP_LOGI(MAIN_TAG, "Tasks running - entering scheduler");
    ESP_LOGI(MAIN_TAG, "========================================");

    // Main task complete - FreeRTOS scheduler takes over
    // All work now happens in the created tasks
}

namespace Tasks {

void blink_task(void* pvParameters) {
    (void)pvParameters;
    ESP_LOGI(TAG, "Blink task started");
    ESP_LOGI(TAG, "  Stack size: %lu bytes", (unsigned long)Config::BLINK_TASK_STACK_SIZE);
    ESP_LOGI(TAG, "  Priority: %d", Config::BLINK_TASK_PRIORITY);

    while (true) {
        BlinkTask::BlinkOperation();
    }
}

void wifi_task(void* pvParameters) {
    (void)pvParameters;
    ESP_LOGI(TAG, "WiFi task started");
    ESP_LOGI(TAG, "  Stack size: %lu bytes", (unsigned long)Config::WIFI_TASK_STACK_SIZE);
    ESP_LOGI(TAG, "  Priority: %d", Config::WIFI_TASK_PRIORITY);

    while (true) {
        WiFiTask::WiFiOperation();
    }
}

void ethernet_task(void* pvParameters) {
    (void)pvParameters;
    ESP_LOGI(TAG, "Ethernet task started");
    ESP_LOGI(TAG, "  Stack size: %lu bytes", (unsigned long)Config::ETH_TASK_STACK_SIZE);
    ESP_LOGI(TAG, "  Priority: %d", Config::ETH_TASK_PRIORITY);

    while (true) {
        EthernetTask::EthernetOperation();
    }
}

void monitor_task(void* pvParameters) {
    (void)pvParameters;
    ESP_LOGI(TAG, "System monitor task started");
    ESP_LOGI(TAG, "  Stack size: %lu bytes", (unsigned long)Config::MONITOR_TASK_STACK_SIZE);
    ESP_LOGI(TAG, "  Priority: %d", Config::MONITOR_TASK_PRIORITY);

    while (true) {
        Monitor::MonitorOperation();
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
