/*
 *  This program is free software; you can redistribute it and/or modify
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
 * AnymaDevice.h
 * Interface for the Anyma device
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBDMX_ANYMADEVICE_H_
#define PLUGINS_USBDMX_ANYMADEVICE_H_

#include <libusb.h>
#include <string>
#include "plugins/usbdmx/UsbDevice.h"
#include "plugins/usbdmx/AnymaOutputPort.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/*
 * A Anyma device
 */
class AnymaDevice: public UsbDevice {
  public:
    AnymaDevice(ola::AbstractPlugin *owner,
                libusb_device *usb_device):
        UsbDevice(owner, "Anyma USB Device", usb_device),
        m_output_port(NULL) {
    }

    string DeviceId() const;

  protected:
    bool StartHook();

  private:
    AnymaOutputPort *m_output_port;
};
}  // usbdmx
}  // plugin
}  // ola
#endif  // PLUGINS_USBDMX_ANYMADEVICE_H_
