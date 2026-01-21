/**
 * @file ethernet_task.cpp
 * @brief Ethernet task operations
 */

#include "freertos/tasks/ethernet_task.hpp"
#include "drivers/ethernet_driver.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace EthernetTask {

namespace {
    bool initialized = false;
} // namespace

void EthernetOperation() {
    if (!initialized) {
        Drivers::Ethernet::initialize();
        initialized = true;
    }

    Drivers::Ethernet::process();
    vTaskDelay(pdMS_TO_TICKS(100));
}

} // namespace EthernetTask
