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
 * JaRuleDevice.cpp
 * A Ja Rule device.
 * Copyright (C) 2015 Simon Newton
 */

#include "plugins/usbdmx/JaRuleDevice.h"

#include <memory>
#include <set>
#include <string>

#include "libs/usb/JaRuleWidget.h"
#include "libs/usb/JaRulePortHandle.h"
#include "plugins/usbdmx/JaRuleOutputPort.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using std::set;
using std::unique_ptr;

JaRuleDevice::JaRuleDevice(ola::AbstractPlugin *owner,
                           ola::usb::JaRuleWidget *widget,
                           const std::string &device_name)
    : Device(owner, device_name),
      m_widget(widget),
      m_device_id(widget->GetUID().ToString()) {
}

bool JaRuleDevice::StartHook() {
  for (uint8_t i = 0; i < m_widget->PortCount(); i++) {
    unique_ptr<JaRuleOutputPort> port(new JaRuleOutputPort(this, i, m_widget));

    if (!port->Init()) {
      continue;
    }

    AddPort(port.release());
  }
  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
