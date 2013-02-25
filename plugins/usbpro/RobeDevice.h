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
 * RobeDevice.h
 * The Robe Universal Interface Device.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_USBPRO_ROBEDEVICE_H_
#define PLUGINS_USBPRO_ROBEDEVICE_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "plugins/usbpro/RobeWidget.h"
#include "plugins/usbpro/UsbSerialDevice.h"

namespace ola {
namespace plugin {
namespace usbpro {


/*
 * The device for a Robe Universe Interface.
 */
class RobeDevice: public UsbSerialDevice {
  public:
    RobeDevice(ola::PluginAdaptor *plugin_adaptor,
               ola::AbstractPlugin *owner,
               const string &name,
               RobeWidget *widget);

    string DeviceId() const { return m_device_id; }

  private:
    string m_device_id;
    class RobeOutputPort *m_output_port;
    class RobeInputPort *m_input_port;
};


/*
 * The Input port
 */
class RobeInputPort: public BasicInputPort {
  public:
    RobeInputPort(RobeDevice *parent,
                  RobeWidget *widget,
                  ola::PluginAdaptor *plugin_adaptor)
        : BasicInputPort(parent, 0, plugin_adaptor),
          m_widget(widget) {
      m_widget->SetDmxCallback(NewCallback(
        static_cast<BasicInputPort*>(this),
        &BasicInputPort::DmxChanged));
    }

    const DmxBuffer &ReadDMX() const {
      return m_widget->FetchDMX();
    }

    void PostSetUniverse(Universe*, Universe *new_universe) {
      if (new_universe)
        m_widget->ChangeToReceiveMode();
    }

    string Description() const { return ""; }

  private:
    string m_path;
    RobeWidget *m_widget;
};


/*
 * For now we just support a single port per device. Some devices may have two
 * ports, but I haven't figured out how to use that yet.
 */
class RobeOutputPort: public BasicOutputPort {
  public:
    RobeOutputPort(RobeDevice *parent, RobeWidget *widget);

    string Description() const { return ""; }
    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback) {
      m_widget->SendRDMRequest(request, callback);
    }

    void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      m_widget->RunFullDiscovery(callback);
    }

    void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      m_widget->RunIncrementalDiscovery(callback);
    }

  private:
    RobeWidget *m_widget;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_ROBEDEVICE_H_
