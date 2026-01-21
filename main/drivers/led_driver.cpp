/**
 * @file led_driver.cpp
 * @brief LED driver implementation
 */

#include "led_driver.hpp"
#include "esp_log.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include <cstring>

static const char* TAG = "LED_Driver";

#define BLINK_GPIO CONFIG_BLINK_GPIO

namespace Drivers {
namespace LED {

namespace {
    bool initialized = false;
    bool current_state = false;
    uint8_t last_r = 16;
    uint8_t last_g = 16;
    uint8_t last_b = 16;

#ifdef CONFIG_BLINK_LED_STRIP
    led_strip_handle_t led_strip = nullptr;
#endif
} // anonymous namespace

bool initialize() {
    if (initialized) {
        ESP_LOGW(TAG, "LED driver already initialized");
        return true;
    }

    ESP_LOGI(TAG, "Initializing LED driver...");

#ifdef CONFIG_BLINK_LED_STRIP
    ESP_LOGI(TAG, "  Type: Addressable LED strip");
    ESP_LOGI(TAG, "  GPIO: %d", BLINK_GPIO);

    led_strip_config_t strip_config;
    memset(&strip_config, 0, sizeof(strip_config));
    strip_config.strip_gpio_num = BLINK_GPIO;
    strip_config.max_leds = 1;

#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config;
    memset(&rmt_config, 0, sizeof(rmt_config));
    rmt_config.resolution_hz = 10 * 1000 * 1000; // 10MHz
    rmt_config.flags.with_dma = false;
    
    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RMT LED strip device: %s", esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "  Backend: RMT");
    
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
    ESP_LOGI(TAG, "  Backend: SPI");
    
#else
    ESP_LOGE(TAG, "Unsupported LED strip backend");
    return false;
#endif

    // Clear LED on init
    led_strip_clear(led_strip);

#elif CONFIG_BLINK_LED_GPIO
    ESP_LOGI(TAG, "  Type: GPIO LED");
    ESP_LOGI(TAG, "  GPIO: %d", BLINK_GPIO);
    
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BLINK_GPIO, 0);

#else
    ESP_LOGE(TAG, "Unsupported LED type");
    return false;
#endif

    initialized = true;
    current_state = false;
    ESP_LOGI(TAG, "LED driver initialized successfully");
    return true;
}

void set_state(bool state) {
    if (!initialized) {
        ESP_LOGW(TAG, "LED driver not initialized");
        return;
    }

    current_state = state;

#ifdef CONFIG_BLINK_LED_STRIP
    if (state) {
        led_strip_set_pixel(led_strip, 0, last_r, last_g, last_b);
        led_strip_refresh(led_strip);
    } else {
        led_strip_clear(led_strip);
    }
#elif CONFIG_BLINK_LED_GPIO
    gpio_set_level(BLINK_GPIO, state ? 1 : 0);
#endif
}

void set_color(uint8_t r, uint8_t g, uint8_t b) {
    if (!initialized) {
        ESP_LOGW(TAG, "LED driver not initialized");
        return;
    }

    last_r = r;
    last_g = g;
    last_b = b;
    current_state = (r > 0) || (g > 0) || (b > 0);

#ifdef CONFIG_BLINK_LED_STRIP
    led_strip_set_pixel(led_strip, 0, r, g, b);
    led_strip_refresh(led_strip);
#elif CONFIG_BLINK_LED_GPIO
    gpio_set_level(BLINK_GPIO, current_state ? 1 : 0);
#endif
}

void get_color(uint8_t* r, uint8_t* g, uint8_t* b) {
    if (r) {
        *r = last_r;
    }
    if (g) {
        *g = last_g;
    }
    if (b) {
        *b = last_b;
    }
}

void toggle() {
    set_state(!current_state);
}

void deinitialize() {
    if (!initialized) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing LED driver");

#ifdef CONFIG_BLINK_LED_STRIP
    if (led_strip) {
        led_strip_clear(led_strip);
        led_strip_del(led_strip);
        led_strip = nullptr;
    }
#elif CONFIG_BLINK_LED_GPIO
    gpio_reset_pin(BLINK_GPIO);
#endif

    initialized = false;
}

} // namespace LED
} // namespace Drivers
