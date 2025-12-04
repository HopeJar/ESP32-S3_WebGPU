/**
 * @file rgb_led.cpp
 * @brief RGB LED device driver implementation
 */

#include "rgb_led.hpp"
#include "esp_log.h"
#include "led_strip.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include <cstring>

static const char* TAG = "RGBLED_Device";
#define RGB_LED_GPIO 6 // Example GPIO, update as needed

namespace Devices {
namespace RGBLED {

namespace {
    bool initialized = false;
    bool current_state = false;
    uint8_t last_r = 0, last_g = 0, last_b = 0;
    led_strip_handle_t led_strip = nullptr;
}

bool initialize() {
    if (initialized) {
        ESP_LOGW(TAG, "RGB LED already initialized");
        return true;
    }
    led_strip_config_t strip_config;
    memset(&strip_config, 0, sizeof(strip_config));
    strip_config.strip_gpio_num = RGB_LED_GPIO;
    strip_config.max_leds = 1;
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config;
    memset(&rmt_config, 0, sizeof(rmt_config));
    rmt_config.resolution_hz = 10 * 1000 * 1000;
    rmt_config.flags.with_dma = false;
    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RMT LED strip device: %s", esp_err_to_name(err));
        return false;
    }
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config;
    memset(&spi_config, 0, sizeof(spi_config));
    spi_config.spi_bus = SPI2_HOST;
    spi_config.flags.with_dma = true;
    esp_err_t err = led_strip_new_spi_device(&strip_config, &spi_config, &led_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create SPI LED strip device: %s", esp_err_to_name(err));
        return false;
    }
#else
    ESP_LOGE(TAG, "Unsupported LED strip backend");
    return false;
#endif
    led_strip_clear(led_strip);
    initialized = true;
    current_state = false;
    ESP_LOGI(TAG, "RGB LED initialized");
    return true;
}

void set_color(uint8_t r, uint8_t g, uint8_t b) {
    if (!initialized || !led_strip) return;
    last_r = r; last_g = g; last_b = b;
    led_strip_set_pixel(led_strip, 0, r, g, b);
    led_strip_refresh(led_strip);
    current_state = true;
}

void set_state(bool state) {
    if (!initialized || !led_strip) return;
    current_state = state;
    if (state) {
        led_strip_set_pixel(led_strip, 0, last_r, last_g, last_b);
        led_strip_refresh(led_strip);
    } else {
        led_strip_clear(led_strip);
    }
}

void deinitialize() {
    if (!initialized) return;
    if (led_strip) {
        led_strip_clear(led_strip);
        led_strip_del(led_strip);
        led_strip = nullptr;
    }
    initialized = false;
}

} // namespace RGBLED
} // namespace Devices
