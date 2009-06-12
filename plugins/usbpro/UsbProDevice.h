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

#ifndef USBPRODEVICE_H
#define USBPRODEVICE_H

#include <string>
#include <deque>
#include <lla/DmxBuffer.h>
#include <lla/network/Socket.h>
#include <llad/Device.h>
#include <llad/PluginAdaptor.h>

#include "messages/UsbProConfigMessages.pb.h"
#include "UsbProWidget.h"
#include "UsbProWidgetListener.h"

namespace lla {
namespace usbpro {

using google::protobuf::RpcController;
using lla::plugin::usbpro::Request;
using lla::network::ConnectedSocket;
using std::deque;

class OutstandingRequest {
  public:
    OutstandingRequest(RpcController *a_controller,
                       string *a_response,
                       google::protobuf::Closure *a_closure):
      controller(a_controller),
      response(a_response),
      closure(a_closure) {}
    RpcController *controller;
    string *response;
    google::protobuf::Closure *closure;
};


class UsbProDevice: public Device, public UsbProWidgetListener {
  public:
    UsbProDevice(const lla::PluginAdaptor *plugin_adaptor,
                 lla::AbstractPlugin *owner,
                 const string &name,
                 const string &dev_path);
    ~UsbProDevice();

    bool Start();
    bool Stop();
    void Configure(RpcController *controller,
                   const string &request,
                   string *response,
                   google::protobuf::Closure *done);

    ConnectedSocket *GetSocket() const;
    bool SendDMX(const DmxBuffer &buffer);
    const DmxBuffer &FetchDMX() const;
    bool ChangeToReceiveMode();

    // callbacks from the widget
    void HandleWidgetDmx();
    void HandleWidgetParameters(uint8_t firmware,
                                uint8_t firmware_high,
                                uint8_t break_time,
                                uint8_t mab_time,
                                uint8_t rate);
    void HandleWidgetSerial(const uint8_t serial[SERIAL_NUMBER_LENGTH]);

  private:
    void HandleParameters(RpcController *controller,
                          const Request *request,
                          string *response,
                          google::protobuf::Closure *done);

    void HandleGetSerial(RpcController *controller,
                         const Request *request,
                         string *response,
                         google::protobuf::Closure *done);

    const lla::PluginAdaptor *m_plugin_adaptor;
    string m_path;
    bool m_enabled;
    bool m_in_shutdown; // set to true if we're shutting down
    UsbProWidget *m_widget;
    deque<OutstandingRequest> m_outstanding_param_requests;
    deque<OutstandingRequest> m_outstanding_serial_requests;

    static const int K_MISSING_PARAM = -1;
};

} //usbpro
} //lla
#endif
