/*
 *  This dmxgram is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This dmxgram is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this dmxgram; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * AnymaDevice.cpp
 * The Anyma usb driver
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <sys/time.h>
#include <string>

#include "ola/Logging.h"
#include "plugins/usbdmx/AnymaDevice.h"

namespace ola {
namespace plugin {
namespace usbdmx {


/*
 * Start this device.
 */
bool AnymaDevice::StartHook() {
  m_output_port = new AnymaOutputPort(this, 0, m_usb_device);
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
string AnymaDevice::DeviceId() const {
  if (m_output_port) {
    return "anyma-" + m_output_port->SerialNumber();
  } else {
    return "";
  }
}
}  // usbdmx
}  // plugin
}  // ola
