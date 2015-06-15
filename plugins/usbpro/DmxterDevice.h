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
 * DmxterDevice.h
 * The Goddard Design Dmxter RDM and miniDmxter
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_DMXTERDEVICE_H_
#define PLUGINS_USBPRO_DMXTERDEVICE_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "plugins/usbpro/DmxterWidget.h"
#include "plugins/usbpro/UsbSerialDevice.h"

namespace ola {
namespace plugin {
namespace usbpro {

/*
 * An DMXter Device
 */
class DmxterDevice: public UsbSerialDevice {
 public:
    DmxterDevice(ola::AbstractPlugin *owner,
                 const std::string &name,
                 DmxterWidget *widget,
                 uint16_t esta_id,
                 uint16_t device_id,
                 uint32_t serial);
    std::string DeviceId() const { return m_device_id; }

 private:
    std::string m_device_id;
};


/*
 * A single Output port per device
 */
class DmxterOutputPort: public BasicOutputPort {
 public:
    DmxterOutputPort(DmxterDevice *parent, DmxterWidget *widget)
        : BasicOutputPort(parent, 0, true, true),
          m_widget(widget) {}

    bool WriteDMX(OLA_UNUSED const DmxBuffer &buffer,
                  OLA_UNUSED uint8_t priority) {
      // this device can't output DMX
      return true;
    }

    void SendRDMRequest(ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback) {
      m_widget->SendRDMRequest(request, callback);
    }

    void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *on_complete) {
      m_widget->RunFullDiscovery(on_complete);
    }

    void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *on_complete) {
      m_widget->RunIncrementalDiscovery(on_complete);
    }

    std::string Description() const { return "RDM Only"; }

 private:
    DmxterWidget *m_widget;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_DMXTERDEVICE_H_
