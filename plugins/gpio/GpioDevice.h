/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * GpioDevice.h
 * The GPIO Device.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_GPIO_GPIODEVICE_H_
#define PLUGINS_GPIO_GPIODEVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "olad/Device.h"
#include "plugins/gpio/GpioDriver.h"

namespace ola {
namespace plugin {
namespace gpio {

/**
 * @brief The GPIO Device
 */
class GpioDevice: public ola::Device {
 public:
  /**
   * @brief Create a new GpioDevice.
   * @param owner The Plugin that owns this device.
   * @param options the options to use for the new device.
   */
  GpioDevice(class GpioPlugin *owner,
             const GpioDriver::Options &options);

  std::string DeviceId() const { return "1"; }

 protected:
  bool StartHook();

 private:
  const GpioDriver::Options m_options;

  DISALLOW_COPY_AND_ASSIGN(GpioDevice);
};
}  // namespace gpio
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_GPIO_GPIODEVICE_H_
