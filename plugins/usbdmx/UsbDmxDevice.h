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
 * UsbDmxDevice.h
 * Interface for the usbdmx device
 * Copyright (C) 2006-2007 Simon Newton
 */

#ifndef PLUGINS_USBDMX_USBDMXDEVICE_H_
#define PLUGINS_USBDMX_USBDMXDEVICE_H_

#include <libusb.h>
#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Device.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/*
 * A UsbDmx device
 */
class UsbDmxDevice: public Device {
  public:
    UsbDmxDevice(ola::AbstractPlugin *owner,
                 libusb_device *dev);
    ~UsbDmxDevice();

    bool Start();
    bool Stop();
    bool AllowLooping() const { return false; }
    bool AllowMultiPortPatching() const { return false; }
    string DeviceId() const { return "1"; }

    bool SendDMX(const DmxBuffer &buffer);

  private:
    enum {VELLEMAN_USB_CHUNK_SIZE = 8};
    enum {COMPRESSED_CHANNEL_COUNT = 6};
    enum {CHANNEL_COUNT = 7};
    bool m_enabled;
    libusb_device *m_usb_device;
    libusb_device_handle *m_usb_handle;

    void SendData(uint8_t *usb_data);
};
}  // usbdmx
}  // plugin
}  // ola
#endif  // PLUGINS_USBDMX_USBDMXDEVICE_H_
