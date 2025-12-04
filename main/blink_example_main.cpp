/**
 * @file blink_example_main.cpp
 * @brief ESP32-S3 Application Entry Point
 * 
 * This is the main entry point for the ESP32-S3 application.
 * It initializes drivers and creates FreeRTOS tasks.
 * 
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 */

#include <cstdio>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "drivers/led_driver.hpp"
#include "freertos/tasks/tasks.hpp"

static const char *TAG = "Main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32-S3 Application Starting");
    ESP_LOGI(TAG, "========================================");

    // Initialize hardware drivers
    ESP_LOGI(TAG, "Initializing drivers...");
    
    if (!Drivers::LED::initialize()) {
        ESP_LOGE(TAG, "Failed to initialize LED driver!");
    }

    ESP_LOGI(TAG, "Driver initialization complete");

    // Create all FreeRTOS tasks
    ESP_LOGI(TAG, "Creating tasks...");
    Tasks::TaskHandles handles = Tasks::create_all_tasks();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Application initialization complete");
    ESP_LOGI(TAG, "Tasks running - entering scheduler");
    ESP_LOGI(TAG, "========================================");

    // Main task complete - FreeRTOS scheduler takes over
    // All work now happens in the created tasks
}
