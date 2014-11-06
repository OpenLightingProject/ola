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
 * ScanlimeDevice.h
 * Interface for the Scanlime device
 * Copyright (C) 2014 Peter Newman
 */

#ifndef PLUGINS_USBDMX_SCANLIMEDEVICE_H_
#define PLUGINS_USBDMX_SCANLIMEDEVICE_H_

#include <libusb.h>
#include <string>
#include "plugins/usbdmx/UsbDevice.h"
#include "plugins/usbdmx/ScanlimeOutputPort.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/*
 * A Scanlime device
 */
class ScanlimeDevice: public UsbDevice {
 public:
    ScanlimeDevice(ola::AbstractPlugin *owner,
                    libusb_device *usb_device,
                    libusb_device_handle *usb_handle,
                    const std::string &serial);

    std::string SerialNumber() const { return m_serial; }
    std::string DeviceId() const;

    static const char EXPECTED_MANUFACTURER[];
    static const char EXPECTED_PRODUCT[];

 protected:
    bool StartHook();

 private:
    ScanlimeOutputPort *m_output_port;
    std::string m_serial;
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_SCANLIMEDEVICE_H_
