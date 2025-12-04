/**
 * @file led2.hpp
 * @brief LED2 device driver interface
 */

#pragma once

namespace Devices {
namespace LED2 {

bool initialize();
void set_state(bool state);
void deinitialize();

} // namespace LED2
} // namespace Devices
