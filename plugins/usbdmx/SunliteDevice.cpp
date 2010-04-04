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
 * SunliteDevice.cpp
 * The USBDMX2 device
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <sys/time.h>

#include "ola/Logging.h"
#include "plugins/usbdmx/SunliteDevice.h"
#include "plugins/usbdmx/SunliteOutputPort.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/*
 * Create a new device
 * @param owner the plugin that owns this device
 * @param usb_device the libusb device
 */
SunliteDevice::SunliteDevice(ola::AbstractPlugin *owner,
                               libusb_device *usb_device):
  UsbDevice(owner, "Sunlite USB Device", usb_device),
  m_enabled(false) {
    libusb_ref_device(usb_device);
}


/*
 * Destroy this device
 */
SunliteDevice::~SunliteDevice() {
  if (m_enabled)
    Stop();
  libusb_unref_device(m_usb_device);
}


/*
 * Start this device.
 */
bool SunliteDevice::Start() {
  SunliteOutputPort *output_port = new SunliteOutputPort(this,
                                                         0,
                                                         m_usb_device);
  if (!output_port->Start()) {
    delete output_port;
    return false;
  }
  AddPort(output_port);
  m_enabled = true;
  return true;
}


/*
 * Stop this device
 */
bool SunliteDevice::Stop() {
  if (!m_enabled)
    return true;

  DeleteAllPorts();

  m_enabled = false;
  return true;
}
}  // usbdmx
}  // plugin
}  // ola
