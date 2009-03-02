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
 * UsbProDevice.cpp
 * UsbPro device
 * Copyright (C) 2006-2007 Simon Newton
 *
 * The device creates two ports, one in and one out, but you can only use one at a time.
 */

#include <iostream>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/service.h>

#include <lla/Closure.h>
#include <llad/logger.h>
#include <llad/Preferences.h>

#include "UsbProDevice.h"
#include "UsbProPort.h"

namespace lla {
namespace plugin {

using google::protobuf::RpcController;
using google::protobuf::Closure;
using lla::plugin::usbpro::Request;
using lla::plugin::usbpro::Reply;
using lla::plugin::usbpro::OutstandingRequest;
using lla::plugin::usbpro::OutstandingParamRequest;
using std::cout;
using std::endl;

/*
 * Create a new device
 *
 * @param owner  the plugin that owns this device
 * @param name  the device name
 * @param dev_path  path to the pro widget
 */
UsbProDevice::UsbProDevice(const lla::PluginAdaptor *plugin_adaptor,
                           lla::AbstractPlugin *owner,
                           const string &name,
                           const string &dev_path):
  Device(owner, name),
  m_plugin_adaptor(plugin_adaptor),
  m_path(dev_path),
  m_enabled(false),
  m_in_shutdown(false),
  m_widget(NULL) {
    m_widget = new UsbProWidget();
}


/*
 * Destroy this device
 */
UsbProDevice::~UsbProDevice() {
  if (m_enabled)
    Stop();

  if (m_widget)
    delete m_widget;
}


/*
 * Start this device
 */
bool UsbProDevice::Start() {
  UsbProPort *port = NULL;
  int ret;

  // connect to the widget
  ret = m_widget->Connect(m_path);

  if (ret) {
    Logger::instance()->log(Logger::WARN, "UsbProPlugin: failed to connect to %s", m_path.c_str());
    return -1;
  }

  m_widget->SetListener(this);

  /* set up ports */
  for (int i=0; i < 2; i++) {
    port = new UsbProPort(this, i);

    if (port)
      AddPort(port);
  }

  m_enabled = true;
  return 0;
}


/*
 * Stop this device
 */
bool UsbProDevice::Stop() {
  if (!m_enabled)
    return false;

  m_in_shutdown = true; // don't allow any more writes
  m_widget->Disconnect();
  DeleteAllPorts();
  m_enabled = false;
  return true;
}


/*
 * return the sd for this device
 */
lla::select_server::ConnectedSocket *UsbProDevice::GetSocket() const {
  return m_widget->GetSocket();
}


/*
 * Send the dmx out the widget
 * called from the UsbProPort
 *
 * @return   0 on success, non 0 on failure
 */
int UsbProDevice::SendDmx(uint8_t *data, int len) {
  return m_widget->SendDmx(data,len);
}


/*
 * Copy the dmx buffer into the arguments
 * Called from the UsbProPort
 *
 * @return   the length of the dmx data copied
 */
int UsbProDevice::FetchDmx(uint8_t *data, int len) const {
  return m_widget->FetchDmx(data, len);
}


/*
 * Handle device config messages
 *
 * @param controller An RpcController
 * @param request the request data
 * @param response the response to return
 * @param done the closure to call once the request is complete
 */
void UsbProDevice::Configure(RpcController *controller,
                             const string &request,
                             string *response,
                             Closure *done) {
  Request request_pb;
  if (!request_pb.ParseFromString(request)) {
    controller->SetFailed("Invalid Request");
    done->Run();
    return;
  }

  switch (request_pb.type()) {
    case lla::plugin::usbpro::Request::USBPRO_PARAMETER_REQUEST:
      HandleParameters(controller, &request_pb, response, done);
      break;
    case lla::plugin::usbpro::Request::USBPRO_SERIAL_REQUEST:
      HandleGetSerial(controller, &request_pb, response, done);
      break;
    default:
      controller->SetFailed("Invalid Request");
      done->Run();
  }
}


/*
 * Put the device back into recv mode
 */
int UsbProDevice::ChangeToReceiveMode() {
  if (m_in_shutdown)
    return 0;
  m_widget->ChangeToReceiveMode();
  return 0;
}


/*
 * Handle a parameter request. This may set some parameters in the widget.
 *
 * If no parameters are set we simply fetch the paramters and return them to
 * the client. If we are setting parameters, we fetch them from the widget,
 * send a SetParam() request and then another GetParam() request in order to
 * return the latest values to the client.
 */
void UsbProDevice::HandleParameters(RpcController *controller,
                                    const Request *request,
                                    string *response,
                                    Closure *done) {

  cout << "in get param" << endl;
  if (!m_widget->GetParameters()) {
    controller->SetFailed("GetParameters failed");
    done->Run();
  } else {
    // TODO: we should time these out if we don't get a response
    OutstandingParamRequest parameters_request;
    parameters_request.controller = controller;
    parameters_request.response = response;
    parameters_request.done = done;
    parameters_request.break_time = K_MISSING_PARAM;
    parameters_request.mab_time = K_MISSING_PARAM;
    parameters_request.rate = K_MISSING_PARAM;

    if (request->has_parameters()) {
      // this results in the set param actions
      if (request->parameters().has_break_time())
        parameters_request.break_time = request->parameters().break_time();
      if (request->parameters().has_mab_time())
        parameters_request.mab_time = request->parameters().mab_time();
      if (request->parameters().has_rate())
        parameters_request.rate = request->parameters().rate();
    }
    m_outstanding_param_requests.push_back(parameters_request);
  }
}


/*
 * Handle a Serial number Configure RPC.
 */
void UsbProDevice::HandleGetSerial(
    RpcController *controller,
    const Request *request __attribute__ ((__unused__)) ,
    string *response,
    Closure *done) {

  if (!m_widget->GetSerial()) {
    controller->SetFailed("GetSerial failed");
    done->Run();
  } else {
    // TODO: we should time these out if we don't get a response
    OutstandingRequest serial_request;
    serial_request.controller = controller;
    serial_request.response = response;
    serial_request.done = done;
    m_outstanding_serial_requests.push_back(serial_request);
  }
}


/*
 * Called when the widget recieves new DMX.
 */
void UsbProDevice::HandleWidgetDmx() {
  AbstractPort *port = GetPort(0);
  port->DmxChanged();
}


/*
 * Called when we get new parameters from the widget.
 */
void UsbProDevice::HandleWidgetParameters(uint8_t firmware,
                                          uint8_t firmware_high,
                                          uint8_t break_time,
                                          uint8_t mab_time,
                                          uint8_t rate) {

  cout << "params returned" << endl;
  if (!m_outstanding_param_requests.empty()) {
    OutstandingParamRequest parameter_request =
      m_outstanding_param_requests.front();
    m_outstanding_param_requests.pop_front();

    if (parameter_request.break_time != K_MISSING_PARAM ||
        parameter_request.mab_time != K_MISSING_PARAM ||
        parameter_request.rate != K_MISSING_PARAM) {
      // we have one or more parameters to set
      SetWidgetParameters(parameter_request, break_time, mab_time, rate);

    } else {
      // just return the response
      Reply reply;
      reply.set_type(lla::plugin::usbpro::Reply::USBPRO_PARAMETER_REPLY);
      lla::plugin::usbpro::ParameterReply *parameters_reply = reply.mutable_parameters();

      parameters_reply->set_firmware_high(firmware_high);
      parameters_reply->set_firmware(firmware);
      parameters_reply->set_break_time(break_time);
      parameters_reply->set_mab_time(mab_time);
      parameters_reply->set_rate(rate);

      reply.SerializeToString(parameter_request.response);
      parameter_request.done->Run();
    }
  }
}


/*
 * Called when the GetSerial request returns
 */
void UsbProDevice::HandleWidgetSerial(
    const uint8_t serial_number[SERIAL_NUMBER_LENGTH]) {
  if (!m_outstanding_serial_requests.empty()) {
    OutstandingRequest serial_request = m_outstanding_serial_requests.front();
    m_outstanding_serial_requests.pop_front();

    Reply reply;
    reply.set_type(lla::plugin::usbpro::Reply::USBPRO_SERIAL_REPLY);
    lla::plugin::usbpro::SerialNumberReply *serial_reply =
      reply.mutable_serial_number();

    serial_reply->set_serial((char*) serial_number, SERIAL_NUMBER_LENGTH);
    reply.SerializeToString(serial_request.response);
    serial_request.done->Run();
  }
}


/*
 * Send a SetParam() request to the widget using the arguments as defaults.
 */
void UsbProDevice::SetWidgetParameters(
    OutstandingParamRequest &outstanding_request,
    uint8_t break_time,
    uint8_t mab_time,
    uint8_t rate) {

  int new_break_time = outstanding_request.break_time == K_MISSING_PARAM ?
    break_time : outstanding_request.break_time;
  int new_mab_time = outstanding_request.mab_time == K_MISSING_PARAM ?
    mab_time : outstanding_request.mab_time;
  int new_rate = outstanding_request.rate == K_MISSING_PARAM ? rate :
    outstanding_request.rate;

  cout << "sending " << new_break_time << " " << new_mab_time << " " << new_rate << endl;
  bool ret = m_widget->SetParameters(NULL, 0, new_break_time, new_mab_time, new_rate);

  if (!ret) {
    outstanding_request.controller->SetFailed("SetParams failed");
    outstanding_request.done->Run();
    return;
  }

  // set a get request
  if (!m_widget->GetParameters()) {
    outstanding_request.controller->SetFailed("GetParam failed");
    outstanding_request.done->Run();
  } else {
    outstanding_request.break_time = K_MISSING_PARAM;
    outstanding_request.mab_time = K_MISSING_PARAM;
    outstanding_request.rate = K_MISSING_PARAM;
    m_outstanding_param_requests.push_back(outstanding_request);
  }
  return;
}


} // plugin
} // lla
