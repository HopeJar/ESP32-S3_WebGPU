/**
 * @file tasks.hpp
 * @brief FreeRTOS task definitions and configuration
 * 
 * This file defines all application tasks with their stack sizes,
 * priorities, and resource requirements clearly documented.
 */

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace Tasks {

// Task configuration constants
namespace Config {
    // Blink LED Task
    constexpr uint32_t BLINK_TASK_STACK_SIZE = 2048;  // 2KB stack
    constexpr UBaseType_t BLINK_TASK_PRIORITY = 5;
    constexpr const char* BLINK_TASK_NAME = "BlinkTask";

    // WiFi Driver Task
    constexpr uint32_t WIFI_TASK_STACK_SIZE = 4096;   // 4KB stack
    constexpr UBaseType_t WIFI_TASK_PRIORITY = 6;
    constexpr const char* WIFI_TASK_NAME = "WiFiTask";

    // Ethernet Driver Task
    constexpr uint32_t ETH_TASK_STACK_SIZE = 4096;    // 4KB stack
    constexpr UBaseType_t ETH_TASK_PRIORITY = 6;
    constexpr const char* ETH_TASK_NAME = "EthernetTask";

    // System Monitor Task
    constexpr uint32_t MONITOR_TASK_STACK_SIZE = 2048; // 2KB stack
    constexpr UBaseType_t MONITOR_TASK_PRIORITY = 3;
    constexpr const char* MONITOR_TASK_NAME = "MonitorTask";
}

/**
 * @brief Task handles for monitoring and control
 */
struct TaskHandles {
    TaskHandle_t blink_task;
    TaskHandle_t wifi_task;
    TaskHandle_t ethernet_task;
    TaskHandle_t monitor_task;
};

/**
 * @brief Initialize and create all application tasks
 * @return TaskHandles structure with all created task handles
 */
TaskHandles create_all_tasks();

/**
 * @brief LED blink task - controls LED blinking via driver
 * @param pvParameters Task parameters (unused)
 */
void blink_task(void* pvParameters);

/**
 * @brief WiFi driver task - manages WiFi connectivity
 * @param pvParameters Task parameters (unused)
 */
void wifi_task(void* pvParameters);

/**
 * @brief Ethernet driver task - manages Ethernet connectivity
 * @param pvParameters Task parameters (unused)
 */
void ethernet_task(void* pvParameters);

/**
 * @brief System monitor task - monitors heap, tasks, etc.
 * @param pvParameters Task parameters (unused)
 */
void monitor_task(void* pvParameters);

} // namespace Tasks
