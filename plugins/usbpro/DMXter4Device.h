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
 * DMXter4Device.h
 * The Goddard Design DMXter4 RDM and miniDMXter4
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_DMXTER4DEVICE_H_
#define PLUGINS_USBPRO_DMXTER4DEVICE_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "plugins/usbpro/UsbDevice.h"

namespace ola {
namespace plugin {
namespace usbpro {


/*
 * An Arduino RGB Mixer Device
 */
class DMXter4Device: public UsbDevice {
  public:
    DMXter4Device(const ola::PluginAdaptor *plugin_adaptor,
                  ola::AbstractPlugin *owner,
                  const string &name,
                  UsbWidget *widget,
                  uint16_t esta_id,
                  uint16_t device_id,
                  uint32_t serial);

    string DeviceId() const { return m_device_id; }

    bool HandleRDMRequest(const ola::rdm::RDMRequest *request);
    void RunRDMDiscovery();
    void SendUIDUpdate();

  private:
    string m_device_id;
};


/*
 * A single Output port per device
 */
class DMXter4DeviceOutputPort: public BasicOutputPort {
  public:
    explicit DMXter4DeviceOutputPort(DMXter4Device *parent)
        : BasicOutputPort(parent, 0),
          m_device(parent) {}

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
      // this device can't output DMX
      return true;
      (void) priority;
      (void) buffer;
    }

    bool HandleRDMRequest(const ola::rdm::RDMRequest *request) {
      return m_device->HandleRDMRequest(request);
    }

    void PostSetUniverse(Universe *old_universe, Universe *new_universe) {
      if (new_universe)
        m_device->SendUIDUpdate();
      (void) old_universe;
    }

    void RunRDMDiscovery() {
      m_device->RunRDMDiscovery();
    }

    string Description() const { return ""; }

  private:
    DMXter4Device *m_device;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_DMXTER4DEVICE_H_
