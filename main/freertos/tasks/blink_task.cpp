/**
 * @file blink_task.cpp
 * @brief Blink task operations
 */

#include "freertos/tasks/blink_task.hpp"
#include "drivers/led_driver.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "logging/logger.hpp"
#include <cstddef>
#include <cstdint>

namespace BlinkTask {

namespace {
    struct RgbColor {
        const char* name;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    const RgbColor kColors[] = {
        {"Red", 255, 0, 0},
        {"Green", 0, 255, 0},
        {"Blue", 0, 0, 255}
    };

    size_t color_idx = 0;
    const Logging::ModuleLogger kLog(Logging::Module::BlinkTask);
} // namespace

void BlinkOperation() {
    const RgbColor& color = kColors[color_idx];
    APP_LOGI(kLog, "LEDs set to %s (R=%u G=%u B=%u)", color.name, color.r, color.g, color.b);
    Drivers::LED::set_color(color.r, color.g, color.b);

    color_idx = (color_idx + 1) % (sizeof(kColors) / sizeof(kColors[0]));
    vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second delay
}

} // namespace BlinkTask
