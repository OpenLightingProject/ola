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
 * DmxterDevice.h
 * The Goddard Design Dmxter RDM and miniDmxter
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_DMXTERDEVICE_H_
#define PLUGINS_USBPRO_DMXTERDEVICE_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "ola/network/SelectServerInterface.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/usbpro/UsbDevice.h"
#include "plugins/usbpro/UsbWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

class DmxterWidget;

/*
 * An Arduino RGB Mixer Device
 */
class DmxterDevice: public UsbDevice {
  public:
    DmxterDevice(ola::network::SelectServerInterface *ss,
                ola::AbstractPlugin *owner,
                const string &name,
                UsbWidget *widget,
                uint16_t esta_id,
                uint16_t device_id,
                uint32_t serial);
    ~DmxterDevice();
    string DeviceId() const { return m_device_id; }
    bool StartHook();

    void HandleRDMRequest(const ola::rdm::RDMRequest *request,
                          ola::rdm::RDMCallback *callback);
    void RunRDMDiscovery();
    void SendUIDUpdate();

  private:
    string m_device_id;
    DmxterWidget *m_dmxter_widget;
};


/*
 * A single Output port per device
 */
class DmxterOutputPort: public BasicOutputPort {
  public:
    explicit DmxterOutputPort(DmxterDevice *parent)
        : BasicOutputPort(parent, 0),
          m_device(parent) {}

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
      // this device can't output DMX
      return true;
      (void) priority;
      (void) buffer;
    }

    void HandleRDMRequest(const ola::rdm::RDMRequest *request,
                          ola::rdm::RDMCallback *callback) {
      m_device->HandleRDMRequest(request, callback);
    }

    void PostSetUniverse(Universe *old_universe, Universe *new_universe) {
      if (new_universe)
        m_device->SendUIDUpdate();
      (void) old_universe;
    }

    void RunRDMDiscovery() {
      m_device->RunRDMDiscovery();
    }

    string Description() const { return "RDM Only"; }

  private:
    DmxterDevice *m_device;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_DMXTERDEVICE_H_
