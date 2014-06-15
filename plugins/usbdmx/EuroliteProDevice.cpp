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
 * EuroliteProDevice.cpp
 * The EurolitePro usb driver
 * Copyright (C) 2011 Simon Newton & Harry F
 * Eurolite Pro USB DMX   ArtNo. 51860120
 */

#include <string>

#include "ola/Logging.h"
#include "plugins/usbdmx/EuroliteProDevice.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;

/*
 * Start this device.
 */
bool EuroliteProDevice::StartHook() {
  m_output_port = new EuroliteProOutputPort(this, 0, m_usb_device);
  if (!m_output_port->Start()) {
    delete m_output_port;
    m_output_port = NULL;
    return false;
  }
  AddPort(m_output_port);
  return true;
}


/*
 * Get the device id
 */
string EuroliteProDevice::DeviceId() const {
  if (m_output_port) {
    return "eurolite-" + m_output_port->SerialNumber();
  } else {
    return "";
  }
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
