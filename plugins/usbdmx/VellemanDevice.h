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
 * VellemanDevice.h
 * Interface for the Velleman device
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBDMX_VELLEMANDEVICE_H_
#define PLUGINS_USBDMX_VELLEMANDEVICE_H_

#include <libusb.h>
#include <string>
#include "plugins/usbdmx/UsbDevice.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/*
 * A Velleman device
 */
class VellemanDevice: public UsbDevice {
  public:
    VellemanDevice(ola::AbstractPlugin *owner,
                   libusb_device *dev);
    ~VellemanDevice();

    bool Start();
    bool Stop();
    bool AllowLooping() const { return false; }
    bool AllowMultiPortPatching() const { return false; }
    string DeviceId() const { return "1"; }

  private:
    bool m_enabled;
    libusb_device *m_usb_device;
};
}  // usbdmx
}  // plugin
}  // ola
#endif  // PLUGINS_USBDMX_VELLEMANDEVICE_H_
