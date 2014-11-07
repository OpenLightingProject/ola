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
 * ScanlimeDevice.cpp
 * The Scanlime usb driver
 * Copyright (C) 2014 Peter Newman
 */

#include <string.h>
#include <sys/time.h>
#include <string>

#include "ola/Logging.h"
#include "plugins/usbdmx/ScanlimeDevice.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;

const char ScanlimeDevice::EXPECTED_MANUFACTURER[] = "scanlime";
const char ScanlimeDevice::EXPECTED_PRODUCT[] = "Fadecandy";


/**
 * New ScanlimeDevice.
 * @param owner the plugin that owns this device
 * @param usb_device a USB device
 * @param usb_handle a claimed handle to the device. Ownership is transferred.
 * @param serial the serial number, may be empty.
 */
ScanlimeDevice::ScanlimeDevice(ola::AbstractPlugin *owner,
                               libusb_device *usb_device,
                               libusb_device_handle *usb_handle,
                               const string &serial)
    : UsbDevice(owner, "Scanlime USB Device", usb_device),
      m_output_port(new ScanlimeOutputPort(this, 0, usb_handle)),
      m_serial(serial) {
}


/*
 * Start this device.
 */
bool ScanlimeDevice::StartHook() {
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
string ScanlimeDevice::DeviceId() const {
  if (!SerialNumber().empty()) {
    return "scanlime-" + SerialNumber();
  } else {
    return "";
  }
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
