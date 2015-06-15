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
 * DmxTriDevice.h
 * The Jese DMX-TRI device.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_DMXTRIDEVICE_H_
#define PLUGINS_USBPRO_DMXTRIDEVICE_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "plugins/usbpro/DmxTriWidget.h"
#include "plugins/usbpro/UsbSerialDevice.h"

namespace ola {
namespace plugin {
namespace usbpro {


/*
 * An DMX TRI Device
 */
class DmxTriDevice: public UsbSerialDevice {
 public:
    DmxTriDevice(ola::AbstractPlugin *owner,
                 const std::string &name,
                 DmxTriWidget *widget,
                 uint16_t esta_id,
                 uint16_t device_id,
                 uint32_t serial,
                 uint16_t firmware_version);
    ~DmxTriDevice() {}

    std::string DeviceId() const { return m_device_id; }
    void PrePortStop();

 private:
    std::string m_device_id;
    DmxTriWidget *m_tri_widget;
};


/*
 * A single output port per device
 */
class DmxTriOutputPort: public BasicOutputPort {
 public:
    DmxTriOutputPort(DmxTriDevice *parent,
                     DmxTriWidget *widget,
                     const std::string &description);

    ~DmxTriOutputPort();

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
    std::string Description() const { return m_description; }

    void SendRDMRequest(ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback) {
      m_tri_widget->SendRDMRequest(request, callback);
    }

    void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      m_tri_widget->RunFullDiscovery(callback);
    }

    void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      m_tri_widget->RunIncrementalDiscovery(callback);
    }

 private:
    DmxTriWidget *m_tri_widget;
    const std::string m_description;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_DMXTRIDEVICE_H_
