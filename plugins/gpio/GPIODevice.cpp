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
 * GPIODevice.cpp
 * The GPIO Device.
 * Copyright (C) 2014 Simon Newton
 */
#include "plugins/gpio/GPIODevice.h"

#include "plugins/gpio/GPIOPort.h"
#include "plugins/gpio/GPIOPlugin.h"
#include "plugins/gpio/GPIODriver.h"

namespace ola {
namespace plugin {
namespace gpio {

/*
 * Create a new device
 */
GPIODevice::GPIODevice(GPIOPlugin *owner,
                       const GPIODriver::Options &options)
    : Device(owner, "General Purpose I/O Device"),
      m_options(options) {
}

bool GPIODevice::StartHook() {
  GPIOOutputPort *port = new GPIOOutputPort(this, m_options);
  if (!port->Init()) {
    delete port;
    return false;
  }
  AddPort(port);
  return true;
}
}  // namespace gpio
}  // namespace plugin
}  // namespace ola
