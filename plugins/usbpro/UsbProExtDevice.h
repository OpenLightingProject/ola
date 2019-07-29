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
 * UsbProExtDevice.h
 * Extended USBPro emulation device support
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_USBPRO_USBPROEXTDEVICE_H_
#define PLUGINS_USBPRO_USBPROEXTDEVICE_H_

#include <string>
#include <sstream>
#include "ola/DmxBuffer.h"
#include "olad/TokenBucket.h"
#include "olad/PluginAdaptor.h"
#include "olad/Port.h"

#include "plugins/usbpro/UsbProExtWidget.h"
#include "plugins/usbpro/UsbSerialDevice.h"
#include "plugins/usbpro/messages/UsbProConfigMessages.pb.h"

namespace ola {
namespace plugin {
namespace usbpro {

/*
 * Extended USBPro device
 */
class UsbProExtDevice: public UsbSerialDevice {
 public:
  UsbProExtDevice(ola::PluginAdaptor *plugin_adaptor,
                    ola::AbstractPlugin *owner,
                    const std::string &name,
                    UsbProExtWidget *widget,
                    uint16_t esta_id,
                    uint16_t device_id,
                    uint32_t serial,
                    uint16_t firmware_version,
                    unsigned int fps_limit);

  std::string DeviceId() const { return m_serial; }
  // both output ports can be bound to the same universe
  bool AllowMultiPortPatching() const { return true; }

  void Configure(ola::rpc::RpcController *controller,
                 const std::string &request,
                 std::string *response,
                 ConfigureCallback *done);

 protected:
  void PrePortStop();

 private:
  void UpdateParams(bool status, const usb_pro_parameters &params);

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

  UsbProExtWidget *m_widget;
  std::string m_serial;

  bool m_got_parameters;
  uint8_t m_break_time;
  uint8_t m_mab_time;
  uint8_t m_rate;
};


/*
 * The Input port
 */
class UsbProExtInputPort: public BasicInputPort {
 public:
  UsbProExtInputPort(UsbProExtDevice *parent,
                       UsbProExtWidget *widget,
                       unsigned int id,
                       ola::PluginAdaptor *plugin_adaptor,
                       const std::string &description)
      : BasicInputPort(parent, id, plugin_adaptor),
        m_description(description),
        m_widget(widget) {}

  const DmxBuffer &ReadDMX() const {
    return m_widget->FetchDMX();
  }

  std::string Description() const { return m_description; }

 private:
  const std::string m_description;
  UsbProExtWidget *m_widget;
};


/*
 * The output port
 */
class UsbProExtOutputPort: public BasicOutputPort {
 public:
  UsbProExtOutputPort(UsbProExtDevice *parent,
                        UsbProExtWidget *widget,
                        unsigned int id,
                        const std::string &description,
                        const TimeStamp *wake_time,
                        unsigned int max_burst,
                        unsigned int rate)
      : BasicOutputPort(parent, id),
		m_port_id_ext(id),
        m_description(description),
        m_widget(widget),
        m_bucket(max_burst, rate, max_burst, *wake_time),
        m_wake_time(wake_time) {}

  bool WriteDMX(const DmxBuffer &buffer, OLA_UNUSED uint8_t priority) {
    if (m_bucket.GetToken(*m_wake_time)) {
      return  m_widget->SendDMX(m_port_id_ext, buffer);
    } else {
      OLA_INFO << "Port rated limited, dropping frame";
    }
    return true;
  }

  std::string Description() const { return m_description; }

 protected:
  const unsigned int m_port_id_ext;
 private:
  const std::string m_description;
  UsbProExtWidget *m_widget;
  TokenBucket m_bucket;
  const TimeStamp *m_wake_time;
//  bool m_primary;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_USBPROEXTDEVICE_H_
