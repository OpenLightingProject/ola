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
#include <lla/select_server/Socket.h>
#include <llad/Device.h>

#include "messages/UsbProConfigMessages.pb.h"
#include "UsbProWidget.h"
#include "UsbProWidgetListener.h"

namespace lla {
namespace plugin {

using google::protobuf::Closure;
using google::protobuf::RpcController;
using lla::plugin::usbpro::Request;
using lla::select_server::ConnectedSocket;
using std::deque;

class UsbProDevice: public Device, public UsbProWidgetListener {
  public:
    UsbProDevice(lla::AbstractPlugin *owner,
                 const string &name,
                 const string &dev_path);
    ~UsbProDevice();

    bool Start();
    bool Stop();
    void Configure(RpcController *controller,
                   const string &request,
                   string *response,
                   Closure *done);

    ConnectedSocket *GetSocket() const;
    int SendDmx(uint8_t *data, int len);
    int FetchDmx(uint8_t *data, int len) const;
    int ChangeToReceiveMode();

    // callbacks from the widget
    void NewDmx();
    void Parameters(uint8_t firmware,
                    uint8_t firmware_high,
                    uint8_t break_time,
                    uint8_t mab_time,
                    uint8_t rate);
    void SerialNumber(const uint8_t serial[SERIAL_NUMBER_LENGTH]);

  private:
    void HandleParameters(RpcController *controller,
                             const Request *request,
                             string *response,
                             Closure *done);

    void HandleGetSerial(RpcController *controller,
                         const Request *request,
                         string *response,
                         Closure *done);

    typedef struct {
      RpcController *controller;
      string *response;
      Closure *done;
    } outstanding_request;

    string m_path;
    bool m_enabled;            // are we enabled
    bool m_in_shutdown;        // set to true if we're shutting down
    UsbProWidget *m_widget;
    deque<outstanding_request> m_outstanding_param_requests;
    deque<outstanding_request> m_outstanding_serial_requests;
};

} //plugin
} //lla
#endif
