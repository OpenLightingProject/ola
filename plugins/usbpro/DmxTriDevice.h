/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * DmxTriDevice.h
 * The Jese DMX-TRI device.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_DMXTRIDEVICE_H_
#define PLUGINS_USBPRO_DMXTRIDEVICE_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "plugins/usbpro/UsbDevice.h"

namespace ola {
namespace plugin {
namespace usbpro {


/*
 * An DMX TRI Device
 */
class DmxTriDevice: public UsbDevice {
  public:
    DmxTriDevice(ola::AbstractPlugin *owner,
                     const string &name,
                     UsbWidget *widget,
                     uint16_t esta_id,
                     uint16_t device_id,
                     uint32_t serial);
    ~DmxTriDevice() {}

    bool Stop();
    bool AllowLooping() const { return false; }
    bool AllowMultiPortPatching() const { return false; }

    string DeviceId() const { return m_device_id; }
    bool SendDMX(const DmxBuffer &buffer) const;

  private:
    string m_device_id;
};


/*
 * A single output port per device
 */
class DmxTriOutputPort: public OutputPort {
  public:
    explicit DmxTriOutputPort(DmxTriDevice *parent)
        : OutputPort(parent, 0),
          m_device(parent) {}

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
      return m_device->SendDMX(buffer);
      (void) priority;
    }
    string Description() const { return ""; }

  private:
    DmxTriDevice *m_device;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_DMXTRIDEVICE_H_
