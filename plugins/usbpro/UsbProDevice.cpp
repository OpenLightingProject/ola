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
 * The device creates two ports, one in and one out, but you can only use one
 * at a time.
 */

#include <iostream>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/service.h>

#include <lla/Closure.h>
#include <lla/Logging.h>
#include <llad/Preferences.h>

#include "UsbProDevice.h"
#include "UsbProPort.h"

namespace lla {
namespace usbpro {

using google::protobuf::RpcController;
using lla::plugin::usbpro::Request;
using lla::plugin::usbpro::Reply;

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

  // connect to the widget
  if (!m_widget->Connect(m_path))
    return false;
  LLA_INFO << "Opened " << m_path;

  m_widget->SetListener(this);

  /* set up ports */
  for (int i = 0; i < 2; i++) {
    port = new UsbProPort(this, i, m_path);

    if (port)
      AddPort(port);
  }

  m_enabled = true;
  return true;
}


/*
 * Stop this device
 */
bool UsbProDevice::Stop() {
  if (!m_enabled)
    return true;

  m_in_shutdown = true; // don't allow any more writes
  m_widget->Disconnect();
  DeleteAllPorts();
  m_enabled = false;
  return true;
}


/*
 * Return the socket for this device
 */
lla::network::ConnectedSocket *UsbProDevice::GetSocket() const {
  return m_widget->GetSocket();
}


/*
 * Send the dmx out the widget
 * @return true on success, false on failure
 */
bool UsbProDevice::SendDMX(const DmxBuffer &buffer){
  return m_widget->SendDMX(buffer);
}


/*
 * Fetch the new DMX data
 * @return the DmxBuffer with the data
 */
const DmxBuffer &UsbProDevice::FetchDMX() const {
  return m_widget->FetchDMX();
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
                             google::protobuf::Closure *done) {
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
 * @return true on success, false on failure
 */
bool UsbProDevice::ChangeToReceiveMode() {
  if (m_in_shutdown)
    return true;
  return m_widget->ChangeToReceiveMode();
}


/*
 * Handle a parameter request. This may set some parameters in the widget.
 *
 * If no parameters are set we simply fetch the parameters and return them to
 * the client. If we are setting parameters, we send a SetParam() request and
 * then another GetParam() request in order to return the latest values to the
 * client.
 */
void UsbProDevice::HandleParameters(RpcController *controller,
                                    const Request *request,
                                    string *response,
                                    google::protobuf::Closure *done) {

  if (request->has_parameters() &&
      (request->parameters().has_break_time() ||
       request->parameters().has_mab_time() ||
       request->parameters().has_rate())) {
    int new_break_time = request->parameters().has_break_time() ?
      request->parameters().break_time() : K_MISSING_PARAM;

    int new_mab_time = request->parameters().has_mab_time() ?
      request->parameters().mab_time() : K_MISSING_PARAM;

    int new_rate = request->parameters().has_rate() ?
      request->parameters().rate() : K_MISSING_PARAM;

    bool ret = m_widget->SetParameters(NULL,
                                       0,
                                       new_break_time,
                                       new_mab_time,
                                       new_rate);

    if (!ret) {
      controller->SetFailed("SetParameters failed");
      done->Run();
      return;
    }
  }

  if (!m_widget->GetParameters()) {
    controller->SetFailed("GetParameters failed");
    done->Run();
  } else {
    // TODO: we should time these out if we don't get a response
    OutstandingRequest parameters_request(controller, response, done);
    m_outstanding_param_requests.push_back(parameters_request);
  }
}


/*
 * Handle a Serial number Configure RPC.
 */
void UsbProDevice::HandleGetSerial(
    RpcController *controller,
    const Request *request __attribute__ ((__unused__)),
    string *response,
    google::protobuf::Closure *done) {

  if (!m_widget->GetSerial()) {
    controller->SetFailed("GetSerial failed");
    done->Run();
  } else {
    // TODO: we should time these out if we don't get a response
    OutstandingRequest serial_request(controller, response, done);
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

  if (!m_outstanding_param_requests.empty()) {
    OutstandingRequest parameter_request =
      m_outstanding_param_requests.front();
    m_outstanding_param_requests.pop_front();

    Reply reply;
    reply.set_type(lla::plugin::usbpro::Reply::USBPRO_PARAMETER_REPLY);
    lla::plugin::usbpro::ParameterReply *parameters_reply =
      reply.mutable_parameters();

    parameters_reply->set_firmware_high(firmware_high);
    parameters_reply->set_firmware(firmware);
    parameters_reply->set_break_time(break_time);
    parameters_reply->set_mab_time(mab_time);
    parameters_reply->set_rate(rate);
    reply.SerializeToString(parameter_request.response);
    parameter_request.closure->Run();
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
    serial_request.closure->Run();
  }
}


} // usbpro
} // lla
