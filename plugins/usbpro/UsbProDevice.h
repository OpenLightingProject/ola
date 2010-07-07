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
 * Interface for the usbpro device
 * Copyright (C) 2006-2007 Simon Newton
 */

#ifndef PLUGINS_USBPRO_USBPRODEVICE_H_
#define PLUGINS_USBPRO_USBPRODEVICE_H_

#include <string>
#include <deque>
#include "ola/DmxBuffer.h"
#include "ola/network/Socket.h"
#include "olad/Device.h"
#include "olad/PluginAdaptor.h"
#include "olad/Port.h"

#include "plugins/usbpro/UsbDevice.h"
#include "plugins/usbpro/UsbWidget.h"
#include "plugins/usbpro/messages/UsbProConfigMessages.pb.h"

namespace ola {
namespace plugin {
namespace usbpro {

using google::protobuf::RpcController;
using ola::plugin::usbpro::Request;
using ola::network::ConnectedSocket;
using std::deque;

/*
 * Outstanding requests to the widget
 */
typedef struct {
  RpcController *controller;
  string *response;
  google::protobuf::Closure *closure;
} OutstandingParamRequest;


/*
 * An Enttec Usb Pro device
 */
class UsbProDevice: public UsbDevice, public WidgetListener {
  public:
    UsbProDevice(const ola::PluginAdaptor *plugin_adaptor,
                 ola::AbstractPlugin *owner,
                 const string &name,
                 UsbWidget *widget,
                 uint16_t esta_id,
                 uint16_t device_id,
                 uint32_t serial,
                 unsigned int fps_limit);
    void HandleMessage(UsbWidget* widget,
                       uint8_t label,
                       unsigned int length,
                       const uint8_t *data);

    string DeviceId() const { return m_serial; }

    void Configure(RpcController *controller,
                   const string &request,
                   string *response,
                   google::protobuf::Closure *done);

    bool SendDMX(const DmxBuffer &buffer);
    const DmxBuffer &FetchDMX() const { return m_input_buffer; }
    bool ChangeToReceiveMode(bool change_only);

  protected:
    void PrePortStop();

  private:
    void HandleParameters(const uint8_t *data, unsigned int length);
    void HandleDMX(const uint8_t *data, unsigned int length);
    void HandleDMXDiff(const uint8_t *data, unsigned int length);
    bool GetParameters() const;

    void HandleParametersRequest(RpcController *controller,
                                 const Request *request,
                                 string *response,
                                 google::protobuf::Closure *done);

    void HandleSerialRequest(RpcController *controller,
                             const Request *request,
                             string *response,
                             google::protobuf::Closure *done);

    bool m_got_parameters;
    bool m_in_shutdown;  // set to true if we're shutting down
    const ola::PluginAdaptor *m_plugin_adaptor;
    string m_serial;
    DmxBuffer m_input_buffer;
    deque<OutstandingParamRequest> m_outstanding_param_requests;

    uint8_t m_break_time;
    uint8_t m_mab_time;
    uint8_t m_rate;

    static const uint8_t REPROGRAM_FIRMWARE_LABEL = 2;
    static const uint8_t PARAMETERS_LABEL = 3;
    static const uint8_t SET_PARAMETERS_LABEL = 4;
    static const uint8_t RECEIVED_DMX_LABEL = 5;
    static const uint8_t DMX_RX_MODE_LABEL = 8;
    static const uint8_t DMX_CHANGED_LABEL = 9;
};


/*
 * The Input port
 */
class UsbProInputPort: public BasicInputPort {
  public:
    UsbProInputPort(UsbProDevice *parent, unsigned int id,
                    const TimeStamp *wake_time, const string &path)
        : BasicInputPort(parent, id, wake_time),
          m_path(path),
          m_device(parent) {}

    const DmxBuffer &ReadDMX() const {
      return m_device->FetchDMX();
    }

    string Description() const {
      return m_path;
    }

  private:
    string m_path;
    UsbProDevice *m_device;
};


/*
 * The output port
 */
class UsbProOutputPort: public BasicOutputPort {
  public:
    UsbProOutputPort(UsbProDevice *parent, unsigned int id, const string &path)
        : BasicOutputPort(parent, id),
          m_path(path),
          m_device(parent) {}

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
      return m_device->SendDMX(buffer);
      (void) priority;
    }

    void PostSetUniverse(Universe *old_universe, Universe *new_universe) {
      if (!new_universe)
        m_device->ChangeToReceiveMode(false);
      (void) old_universe;
    }

    string Description() const { return m_path; }

  private:
    string m_path;
    UsbProDevice *m_device;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_USBPRODEVICE_H_
