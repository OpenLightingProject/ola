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
 * UsbProDevice.h
 * Interface for the Enttec USB Pro device
 * Copyright (C) 2006-2007 Simon Newton
 */

#ifndef PLUGINS_USBPRO_USBPRODEVICE_H_
#define PLUGINS_USBPRO_USBPRODEVICE_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "olad/PluginAdaptor.h"
#include "olad/Port.h"

#include "plugins/usbpro/UsbDevice.h"
#include "plugins/usbpro/UsbWidget.h"
#include "plugins/usbpro/UsbProWidget.h"
#include "plugins/usbpro/messages/UsbProConfigMessages.pb.h"

namespace ola {
namespace plugin {
namespace usbpro {

using google::protobuf::RpcController;
using ola::plugin::usbpro::Request;


/*
 * An Enttec Usb Pro device
 */
class UsbProDevice: public UsbDevice {
  public:
    UsbProDevice(ola::PluginAdaptor *plugin_adaptor,
                 ola::AbstractPlugin *owner,
                 const string &name,
                 UsbWidget *widget,
                 uint16_t esta_id,
                 uint16_t device_id,
                 uint32_t serial,
                 unsigned int fps_limit);
    ~UsbProDevice();

    string DeviceId() const { return m_serial; }

    void Configure(RpcController *controller,
                   const string &request,
                   string *response,
                   google::protobuf::Closure *done);

  protected:
    void PrePortStop();

  private:
    void UpdateParams(bool status, const usb_pro_parameters &params);

    void HandleParametersRequest(RpcController *controller,
                                 const Request *request,
                                 string *response,
                                 google::protobuf::Closure *done);

    void HandleParametersResponse(RpcController *controller,
                                  string *response,
                                  google::protobuf::Closure *done,
                                  bool status,
                                  const usb_pro_parameters &params);

    void HandleSerialRequest(RpcController *controller,
                             const Request *request,
                             string *response,
                             google::protobuf::Closure *done);

    UsbProWidget *m_pro_widget;
    string m_serial;

    bool m_got_parameters;
    uint8_t m_break_time;
    uint8_t m_mab_time;
    uint8_t m_rate;
};


/*
 * The Input port
 */
class UsbProInputPort: public BasicInputPort {
  public:
    UsbProInputPort(UsbProDevice *parent,
                    UsbProWidget *widget,
                    unsigned int id,
                    ola::PluginAdaptor *plugin_adaptor,
                    const string &path)
        : BasicInputPort(parent, id, plugin_adaptor),
          m_path(path),
          m_widget(widget) {}

    const DmxBuffer &ReadDMX() const {
      return m_widget->FetchDMX();
    }

    string Description() const { return m_path; }

  private:
    string m_path;
    UsbProWidget *m_widget;
};


/*
 * The output port
 */
class UsbProOutputPort: public BasicOutputPort {
  public:
    UsbProOutputPort(UsbProDevice *parent,
                     UsbProWidget *widget,
                     unsigned int id,
                     const string &path)
        : BasicOutputPort(parent, id),
          m_path(path),
          m_widget(widget) {}

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
      return m_widget->SendDMX(buffer);
      (void) priority;
    }

    void PostSetUniverse(Universe *old_universe, Universe *new_universe) {
      if (!new_universe)
        m_widget->ChangeToReceiveMode(false);
      (void) old_universe;
    }

    string Description() const { return m_path; }

  private:
    string m_path;
    UsbProWidget *m_widget;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_USBPRODEVICE_H_
