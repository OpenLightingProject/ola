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
 * UsbProDevice.h
 * A Enttec USB Pro device
 * Copyright (C) 2006-2007 Simon Newton
 */

#ifndef PLUGINS_USBPRO_USBPRODEVICE_H_
#define PLUGINS_USBPRO_USBPRODEVICE_H_

#include <string>
#include <vector>
#include "ola/DmxBuffer.h"
#include "olad/TokenBucket.h"
#include "olad/PluginAdaptor.h"
#include "olad/Port.h"

#include "plugins/usbpro/EnttecUsbProWidget.h"
#include "plugins/usbpro/UsbSerialDevice.h"
#include "plugins/usbpro/messages/UsbProConfigMessages.pb.h"

namespace ola {
namespace plugin {
namespace usbpro {

using google::protobuf::RpcController;
using ola::plugin::usbpro::Request;


/*
 * An Enttec Usb Pro device
 */
class UsbProDevice: public UsbSerialDevice {
  public:
    UsbProDevice(ola::PluginAdaptor *plugin_adaptor,
                 ola::AbstractPlugin *owner,
                 const string &name,
                 EnttecUsbProWidget *widget,
                 uint32_t serial,
                 unsigned int fps_limit);

    string DeviceId() const { return m_serial; }

    void Configure(RpcController *controller,
                   const string &request,
                   string *response,
                   google::protobuf::Closure *done);

  protected:
    void PrePortStop();

  private:
    struct PortParams {
      bool got_parameters;
      uint8_t break_time;
      uint8_t mab_time;
      uint8_t rate;
    };

    void UpdateParams(unsigned int port_id, bool status,
                      const usb_pro_parameters &params);

    void HandleParametersRequest(RpcController *controller,
                                 const Request *request,
                                 string *response,
                                 google::protobuf::Closure *done);

    void HandleParametersResponse(RpcController *controller,
                                  string *response,
                                  google::protobuf::Closure *done,
                                  unsigned int port_id,
                                  bool status,
                                  const usb_pro_parameters &params);

    void HandleSerialRequest(RpcController *controller,
                             const Request *request,
                             string *response,
                             google::protobuf::Closure *done);

    void HandlePortAssignmentRequest(RpcController *controller,
                                     const Request *request,
                                     string *response,
                                     google::protobuf::Closure *done);

    void HandlePortAssignmentResponse(RpcController *controller,
                                      string *response,
                                      google::protobuf::Closure *done,
                                      bool status,
                                      uint8_t port1_assignment,
                                      uint8_t port2_assignment);

    static string SerialToString(uint32_t serial);

    EnttecUsbProWidget *m_pro_widget;
    string m_serial;
    vector<PortParams> m_port_params;
};


/*
 * The Input port
 */
class UsbProInputPort: public BasicInputPort {
  public:
    // The EnttecPort is owner by the caller.
    UsbProInputPort(UsbProDevice *parent,
                    EnttecPort *port,
                    unsigned int id,
                    ola::PluginAdaptor *plugin_adaptor,
                    const string &serial)
        : BasicInputPort(parent, id, plugin_adaptor),
          m_serial(serial),
          m_port(port) {}

    const DmxBuffer &ReadDMX() const {
      return m_port->FetchDMX();
    }

    string Description() const { return "Serial #: " + m_serial; }

  private:
    const string m_serial;
    EnttecPort *m_port;
};


/*
 * The output port
 */
class UsbProOutputPort: public BasicOutputPort {
  public:
    // The EnttecPort is owner by the caller.
    UsbProOutputPort(UsbProDevice *parent,
                     EnttecPort *port,
                     unsigned int id,
                     const string &serial,
                     const TimeStamp *wake_time,
                     unsigned int max_burst,
                     unsigned int rate)
        : BasicOutputPort(parent, id, true, true),
          m_serial(serial),
          m_port(port),
          m_bucket(max_burst, rate, max_burst, *wake_time),
          m_wake_time(wake_time) {}

    bool WriteDMX(const DmxBuffer &buffer, uint8_t) {
      if (m_bucket.GetToken(*m_wake_time))
        return m_port->SendDMX(buffer);
      else
        OLA_INFO << "Port rated limited, dropping frame";
      return true;
    }

    void PostSetUniverse(Universe*, Universe *new_universe) {
      if (!new_universe)
        m_port->ChangeToReceiveMode(false);
    }

    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback) {
      m_port->SendRDMRequest(request, callback);
    }

    void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      m_port->RunFullDiscovery(callback);
    }

    void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      m_port->RunIncrementalDiscovery(callback);
    }

    string Description() const { return "Serial #: " + m_serial; }

  private:
    const string m_serial;
    EnttecPort *m_port;
    TokenBucket m_bucket;
    const TimeStamp *m_wake_time;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_USBPRODEVICE_H_
