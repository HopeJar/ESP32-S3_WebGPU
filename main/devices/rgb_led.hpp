/**
 * @file rgb_led.hpp
 * @brief RGB LED device driver interface
 */

#pragma once
#include <cstdint>

namespace Devices {
namespace RGBLED {

bool initialize();
void set_color(uint8_t r, uint8_t g, uint8_t b);
void set_state(bool state); // on/off
void deinitialize();

} // namespace RGBLED
} // namespace Devices
