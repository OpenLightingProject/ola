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
 * UsbDevice.h
 * Interface for the generic usb device
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBDMX_USBDEVICE_H_
#define PLUGINS_USBDMX_USBDEVICE_H_

#include <libusb.h>
#include <string>
#include "olad/Device.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/*
 * A Usb device, this is just like the generic Device class but it has a
 * Start() method as well to do the USB setup.
 */
class UsbDevice: public ola::Device {
  public:
    UsbDevice(ola::AbstractPlugin *owner,
              const string &name,
              libusb_device *device):
        Device(owner, name),
        m_usb_device(device) {
      libusb_ref_device(device);
    }
    virtual ~UsbDevice() {
      libusb_unref_device(m_usb_device);
    }

  protected:
    libusb_device *m_usb_device;
};
}  // usbdmx
}  // plugin
}  // ola
#endif  // PLUGINS_USBDMX_USBDEVICE_H_
