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
 * GPIOPort.h
 * An OLA I/O Port.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_GPIO_GPIOPORT_H_
#define PLUGINS_GPIO_GPIOPORT_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "plugins/gpio/GPIODevice.h"
#include "plugins/gpio/GPIODriver.h"

namespace ola {
namespace plugin {
namespace gpio {

class GPIOOutputPort: public BasicOutputPort {
 public:
  GPIOOutputPort(GPIODevice *parent,
                 const GPIODriver::Options &options);
  ~GPIOOutputPort() {}

  bool Init();

  std::string Description() const;
  bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

 private:
  std::auto_ptr<GPIODriver> m_driver;

  DISALLOW_COPY_AND_ASSIGN(GPIOOutputPort);
};
}  // namespace gpio
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_GPIO_GPIOPORT_H_
