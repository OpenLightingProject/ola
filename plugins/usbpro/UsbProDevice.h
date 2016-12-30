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
 * UsbProDevice.h
 * A Enttec USB Pro device
 * Copyright (C) 2006 Simon Newton
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

/*
 * An Enttec Usb Pro device
 */
class UsbProDevice: public UsbSerialDevice {
 public:
  UsbProDevice(ola::PluginAdaptor *plugin_adaptor,
               ola::AbstractPlugin *owner,
               const std::string &name,
               EnttecUsbProWidget *widget,
               uint32_t serial,
               uint16_t firmware_version,
               unsigned int fps_limit);

  std::string DeviceId() const { return m_serial; }

  void Configure(ola::rpc::RpcController *controller,
                 const std::string &request,
                 std::string *response,
                 ConfigureCallback *done);

  bool AllowMultiPortPatching() const { return true; }

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

  void HandleParametersRequest(ola::rpc::RpcController *controller,
                               const ola::plugin::usbpro::Request *request,
                               std::string *response,
                               ConfigureCallback *done);

  void HandleParametersResponse(ola::rpc::RpcController *controller,
                                std::string *response,
                                ConfigureCallback *done,
                                unsigned int port_id,
                                bool status,
                                const usb_pro_parameters &params);

  void HandleSerialRequest(ola::rpc::RpcController *controller,
                           const ola::plugin::usbpro::Request *request,
                           std::string *response,
                           ConfigureCallback *done);

  void HandlePortAssignmentRequest(ola::rpc::RpcController *controller,
                                   const ola::plugin::usbpro::Request *request,
                                   std::string *response,
                                   ConfigureCallback *done);

  void HandlePortAssignmentResponse(ola::rpc::RpcController *controller,
                                    std::string *response,
                                    ConfigureCallback *done,
                                    bool status,
                                    uint8_t port1_assignment,
                                    uint8_t port2_assignment);

  static std::string SerialToString(uint32_t serial);

  EnttecUsbProWidget *m_pro_widget;
  std::string m_serial;
  std::vector<PortParams> m_port_params;
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
                  const std::string &description = "")
      : BasicInputPort(parent, id, plugin_adaptor),
        m_description(description),
        m_port(port) {}

  const DmxBuffer &ReadDMX() const {
    return m_port->FetchDMX();
  }

  std::string Description() const { return m_description; }

 private:
  const std::string m_description;
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
                   const std::string &description,
                   const TimeStamp *wake_time,
                   unsigned int max_burst,
                   unsigned int rate)
      : BasicOutputPort(parent, id, port->SupportsRDM(), port->SupportsRDM()),
        m_description(description),
        m_port(port),
        m_bucket(max_burst, rate, max_burst, *wake_time),
        m_wake_time(wake_time) {}

  bool WriteDMX(const DmxBuffer &buffer, uint8_t) {
    if (m_bucket.GetToken(*m_wake_time)) {
      return m_port->SendDMX(buffer);
    } else {
      OLA_INFO << "Port rated limited, dropping frame";
    }
    return true;
  }

  void PostSetUniverse(Universe*, Universe *new_universe) {
    if (!new_universe) {
      m_port->ChangeToReceiveMode(false);
    }
  }

  void SendRDMRequest(ola::rdm::RDMRequest *request,
                      ola::rdm::RDMCallback *callback) {
    m_port->SendRDMRequest(request, callback);
  }

  void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
    m_port->RunFullDiscovery(callback);
  }

  void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
    m_port->RunIncrementalDiscovery(callback);
  }

  std::string Description() const { return m_description; }

 private:
  const std::string m_description;
  EnttecPort *m_port;
  TokenBucket m_bucket;
  const TimeStamp *m_wake_time;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_USBPRODEVICE_H_
