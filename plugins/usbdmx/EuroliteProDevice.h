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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * EuroliteProDevice.h
 * Interface for the EurolitePro device
 * Copyright (C) 2011 Simon Newton & Harry F
 * Eurolite Pro USB DMX   ArtNo. 51860120
 */

#ifndef PLUGINS_USBDMX_EUROLITEPRODEVICE_H_
#define PLUGINS_USBDMX_EUROLITEPRODEVICE_H_

#include <libusb.h>
#include <string>
#include "plugins/usbdmx/UsbDevice.h"
#include "plugins/usbdmx/EuroliteProOutputPort.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/*
 * A EurolitePro device
 */
class EuroliteProDevice: public UsbDevice {
  public:
    EuroliteProDevice(ola::AbstractPlugin *owner,
                      libusb_device *usb_device):
        UsbDevice(owner, "EurolitePro USB Device", usb_device),
        m_output_port(NULL) {
    }

    string DeviceId() const;

  protected:
    bool StartHook();

  private:
    EuroliteProOutputPort *m_output_port;
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_EUROLITEPRODEVICE_H_
