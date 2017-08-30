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
 * GpioPort.cpp
 * An OLA GPIO Port.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/gpio/GpioPort.h"

#include <sstream>
#include <string>
#include <vector>

#include "ola/StringUtils.h"
#include "plugins/gpio/GpioDriver.h"

namespace ola {
namespace plugin {
namespace gpio {

using std::string;
using std::vector;

GpioOutputPort::GpioOutputPort(GpioDevice *parent,
                               const GpioDriver::Options &options)
    : BasicOutputPort(parent, 1),
      m_driver(new GpioDriver(options)) {
}

bool GpioOutputPort::Init() {
  return m_driver->Init();
}

string GpioOutputPort::Description() const {
  vector<uint16_t> pins = m_driver->PinList();
  return "Pins " + ola::StringJoin(", ", pins);
}

bool GpioOutputPort::WriteDMX(const DmxBuffer &buffer,
                              OLA_UNUSED uint8_t priority) {
  return m_driver->SendDmx(buffer);
}
}  // namespace gpio
}  // namespace plugin
}  // namespace ola
