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
 * DMXCProjectsNodleU1Device.cpp
 * A DMXCProjectsNodleU1 device that creates an input and an output port.
 * Copyright (C) 2015 Stefan Krupop
 */

#include "plugins/usbdmx/DMXCProjectsNodleU1Device.h"

#include <string>
#include "plugins/usbdmx/DMXCProjectsNodleU1Port.h"
#include "plugins/usbdmx/GenericOutputPort.h"

namespace ola {
namespace plugin {
namespace usbdmx {

DMXCProjectsNodleU1Device::DMXCProjectsNodleU1Device(
    ola::AbstractPlugin *owner,
    DMXCProjectsNodleU1 *widget,
    const std::string &device_name,
    const std::string &device_id,
    PluginAdaptor *plugin_adaptor)
    : Device(owner, device_name),
      m_device_id(device_id),
      m_out_port(),
      m_in_port() {
  unsigned int mode = widget->Mode();

  if (mode & DMXCProjectsNodleU1::OUTPUT_ENABLE_MASK) {  // output port active
    m_out_port.reset(new GenericOutputPort(this, 0, widget));
  }

  if (mode & DMXCProjectsNodleU1::INPUT_ENABLE_MASK) {  // input port active
    m_in_port.reset(new DMXCProjectsNodleU1InputPort(this, 0, plugin_adaptor,
                                                     widget));
  }
}

bool DMXCProjectsNodleU1Device::StartHook() {
  if (m_out_port.get()) {
    AddPort(m_out_port.release());
  }
  if (m_in_port.get()) {
    AddPort(m_in_port.release());
  }
  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
