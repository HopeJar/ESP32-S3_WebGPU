// FreeRTOS Blink Task Example
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void task_blink(void *pvParameters) {
    while (1) {
        // Toggle LED here
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
