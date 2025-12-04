/**
 * @file led1.hpp
 * @brief LED1 device driver interface
 */

#pragma once

namespace Devices {
namespace LED1 {

bool initialize();
void set_state(bool state);
void deinitialize();

} // namespace LED1
} // namespace Devices
