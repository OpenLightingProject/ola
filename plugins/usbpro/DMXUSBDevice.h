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
 * DMXUSBDevice.h
 * A DMXUSB Device (based on DMX King Ultra DMX Pro code by Simon Newton)
 * Copyright (C) 2019 Perry Naseck (DaAwesomeP)
 */

#ifndef PLUGINS_USBPRO_DMXUSBDEVICE_H_
#define PLUGINS_USBPRO_DMXUSBDEVICE_H_

#include <string>
#include <sstream>
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "olad/TokenBucket.h"
#include "olad/PluginAdaptor.h"
#include "olad/Port.h"

#include "plugins/usbpro/DMXUSBWidget.h"
#include "plugins/usbpro/UsbSerialDevice.h"
#include "plugins/usbpro/messages/UsbProConfigMessages.pb.h"

namespace ola {
namespace plugin {
namespace usbpro {

/*
 * An DMXUSB device
 */
class DMXUSBDevice: public UsbSerialDevice {
 public:
  DMXUSBDevice(ola::PluginAdaptor *plugin_adaptor,
                    ola::AbstractPlugin *owner,
                    const std::string &name,
                    DMXUSBWidget *widget,
                    uint16_t esta_id,
                    uint16_t device_id,
                    uint32_t serial,
                    uint16_t firmware_version,
                    unsigned int fps_limit);

  std::string DeviceId() const { return m_serial; }
  // all output ports can be bound to the same universe
  bool AllowMultiPortPatching() const { return true; }

  void Configure(ola::rpc::RpcController *controller,
                 const std::string &request,
                 std::string *response,
                 ConfigureCallback *done);

 protected:
  void PrePortStop();

 private:
  void UpdateParams(bool status, const usb_pro_parameters &params);
  void UpdateExtendedParams(bool status,
                            const dmxusb_extended_parameters &params);
  void SetupPorts();

  void HandleParametersRequest(ola::rpc::RpcController *controller,
                               const ola::plugin::usbpro::Request *request,
                               std::string *response,
                               ConfigureCallback *done);

  void HandleParametersResponse(ola::rpc::RpcController *controller,
                                std::string *response,
                                ConfigureCallback *done,
                                bool status,
                                const usb_pro_parameters &params);

  void HandleSerialRequest(ola::rpc::RpcController *controller,
                           const ola::plugin::usbpro::Request *request,
                           std::string *response,
                           ConfigureCallback *done);

  ola::PluginAdaptor *m_plugin_adaptor;
  DMXUSBWidget *m_dmxusb_widget;
  std::string m_serial;
  std::ostringstream m_str;

  bool m_got_parameters;
  bool m_got_extended_parameters;
  uint8_t m_break_time;
  uint8_t m_mab_time;
  uint8_t m_rate;
  uint8_t m_out_ports;
  uint8_t m_in_ports;
  unsigned int m_fps_limit;
};


/*
 * The output ports
 */
class DMXUSBOutputPort: public BasicOutputPort {
 public:
  DMXUSBOutputPort(DMXUSBDevice *parent,
                        DMXUSBWidget *widget,
                        unsigned int id,
                        const std::string &description,
                        const TimeStamp *wake_time,
                        unsigned int max_burst,
                        unsigned int rate,
                        unsigned int port)
      : BasicOutputPort(parent, id),
        m_description(description),
        m_widget(widget),
        m_bucket(max_burst, rate, max_burst, *wake_time),
        m_wake_time(wake_time),
        m_port(port) {}

  bool WriteDMX(const DmxBuffer &buffer, OLA_UNUSED uint8_t priority) {
    if (m_bucket.GetToken(*m_wake_time)) {
      return m_widget->SendDMXPort(m_port, buffer);
    } else {
      OLA_INFO << "DMXUSB port " << m_port << " rated limited, dropping frame";
    }
    return true;
  }

  std::string Description() const { return m_description; }

 private:
  const std::string m_description;
  DMXUSBWidget *m_widget;
  TokenBucket m_bucket;
  const TimeStamp *m_wake_time;
  unsigned int m_port;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_DMXUSBDEVICE_H_
