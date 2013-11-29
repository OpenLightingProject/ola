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
 * UltraDMXProDevice.h
 * A DMX King Ultra DMX Pro Device
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_USBPRO_ULTRADMXPRODEVICE_H_
#define PLUGINS_USBPRO_ULTRADMXPRODEVICE_H_

#include <string>
#include <sstream>
#include "ola/DmxBuffer.h"
#include "olad/TokenBucket.h"
#include "olad/PluginAdaptor.h"
#include "olad/Port.h"

#include "plugins/usbpro/UltraDMXProWidget.h"
#include "plugins/usbpro/UsbSerialDevice.h"
#include "plugins/usbpro/messages/UsbProConfigMessages.pb.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::plugin::usbpro::Request;


/*
 * An Ultra DMX Pro device
 */
class UltraDMXProDevice: public UsbSerialDevice {
 public:
    UltraDMXProDevice(ola::PluginAdaptor *plugin_adaptor,
                      ola::AbstractPlugin *owner,
                      const string &name,
                      UltraDMXProWidget *widget,
                      uint16_t esta_id,
                      uint16_t device_id,
                      uint32_t serial,
                      unsigned int fps_limit);

    string DeviceId() const { return m_serial; }
    // both output ports can be bound to the same universe
    bool AllowMultiPortPatching() const { return true; }

    void Configure(ola::rpc::RpcController *controller,
                   const string &request,
                   string *response,
                   ConfigureCallback *done);

 protected:
    void PrePortStop();

 private:
    void UpdateParams(bool status, const usb_pro_parameters &params);

    void HandleParametersRequest(ola::rpc::RpcController *controller,
                                 const Request *request,
                                 string *response,
                                 ConfigureCallback *done);

    void HandleParametersResponse(ola::rpc::RpcController *controller,
                                  string *response,
                                  ConfigureCallback *done,
                                  bool status,
                                  const usb_pro_parameters &params);

    void HandleSerialRequest(ola::rpc::RpcController *controller,
                             const Request *request,
                             string *response,
                             ConfigureCallback *done);

    UltraDMXProWidget *m_ultra_widget;
    string m_serial;

    bool m_got_parameters;
    uint8_t m_break_time;
    uint8_t m_mab_time;
    uint8_t m_rate;
};


/*
 * The Input port
 */
class UltraDMXProInputPort: public BasicInputPort {
 public:
    UltraDMXProInputPort(UltraDMXProDevice *parent,
                         UltraDMXProWidget *widget,
                         unsigned int id,
                         ola::PluginAdaptor *plugin_adaptor,
                         const string &serial)
        : BasicInputPort(parent, id, plugin_adaptor),
          m_serial(serial),
          m_widget(widget) {}

    const DmxBuffer &ReadDMX() const {
      return m_widget->FetchDMX();
    }

    string Description() const {
      std::stringstream str;
      str << "Serial " << m_serial;
      return str.str();
    }

 private:
    string m_serial;
    UltraDMXProWidget *m_widget;
};


/*
 * The output port, we have two of these per device.
 */
class UltraDMXProOutputPort: public BasicOutputPort {
 public:
    UltraDMXProOutputPort(UltraDMXProDevice *parent,
                          UltraDMXProWidget *widget,
                          unsigned int id,
                          string serial,
                          const TimeStamp *wake_time,
                          unsigned int max_burst,
                          unsigned int rate,
                          bool primary)
        : BasicOutputPort(parent, id),
          m_serial(serial),
          m_widget(widget),
          m_bucket(max_burst, rate, max_burst, *wake_time),
          m_wake_time(wake_time),
          m_primary(primary) {}

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
      if (m_bucket.GetToken(*m_wake_time))
        if (m_primary)
          return m_widget->SendDMX(buffer);
        else
          return m_widget->SendSecondaryDMX(buffer);
      else
        OLA_INFO << "Port rated limited, dropping frame";
      return true;
      (void) priority;
    }

    string Description() const {
      std::stringstream str;
      str << "Serial " << m_serial << ", " <<
        (m_primary ? "Port 1" : "Port 2");
      return str.str();
    }

 private:
    string m_serial;
    UltraDMXProWidget *m_widget;
    TokenBucket m_bucket;
    const TimeStamp *m_wake_time;
    bool m_primary;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_ULTRADMXPRODEVICE_H_
