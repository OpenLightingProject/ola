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
 * UltraDMXProDevice.cpp
 * A DMX King Ultra DMX Pro Device
 * Copyright (C) 2011 Simon Newton
 *
 * This device creates 3 ports, 1 input and 2 output.
 */

#include <iomanip>
#include <iostream>
#include <string>

#include "common/rpc/RpcController.h"
#include "ola/BaseTypes.h"
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "plugins/usbpro/UltraDMXProDevice.h"
#include "plugins/usbpro/UsbProWidgetDetector.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::plugin::usbpro::Request;
using ola::plugin::usbpro::Reply;
using ola::rpc::RpcController;

/*
 * Create a new device
 * @param owner  the plugin that owns this device
 * @param name  the device name
 * @param dev_path  path to the pro widget
 */
UltraDMXProDevice::UltraDMXProDevice(ola::PluginAdaptor *plugin_adaptor,
                                     ola::AbstractPlugin *owner,
                                     const string &name,
                                     UltraDMXProWidget *widget,
                                     uint16_t esta_id,
                                     uint16_t device_id,
                                     uint32_t serial,
                                     unsigned int fps_limit):
    UsbSerialDevice(owner, name, widget),
    m_ultra_widget(widget),
    m_serial(),
    m_got_parameters(false) {
  std::stringstream str;
  str << std::setfill('0');
  uint8_t *ptr = reinterpret_cast<uint8_t*>(&serial);
  for (int i = UsbProWidgetInformation::SERIAL_LENGTH - 1; i >= 0; i--) {
    int digit = (10 * (ptr[i] & 0xf0) >> 4) + (ptr[i] & 0x0f);
    str <<  std::setw(2)  << digit;
  }
  m_serial = str.str();

  m_ultra_widget->GetParameters(NewSingleCallback(
    this,
    &UltraDMXProDevice::UpdateParams));

  UltraDMXProInputPort *input_port = new UltraDMXProInputPort(
      this,
      m_ultra_widget,
      0,
      plugin_adaptor,
      m_serial);

  m_ultra_widget->SetDMXCallback(
      NewCallback(
        static_cast<InputPort*>(input_port),
        &InputPort::DmxChanged));
  AddPort(input_port);

  // add the primary port
  OutputPort *output_port = new UltraDMXProOutputPort(
      this,
      m_ultra_widget,
      0,
      m_serial,
      plugin_adaptor->WakeUpTime(),
      5,  // allow up to 5 burst frames
      fps_limit,
      true);
  AddPort(output_port);

  // add the secondary port
  output_port = new UltraDMXProOutputPort(
      this,
      m_ultra_widget,
      1,
      m_serial,
      plugin_adaptor->WakeUpTime(),
      5,  // allow up to 5 burst frames
      fps_limit,
      false);
  AddPort(output_port);

  Start();  // this does nothing but set IsEnabled() to true
  (void) esta_id;
  (void) device_id;
}


/*
 * Stop this device
 */
void UltraDMXProDevice::PrePortStop() {
  m_ultra_widget->Stop();
}


/*
 * Handle device config messages
 * @param controller An RpcController
 * @param request the request data
 * @param response the response to return
 * @param done the closure to call once the request is complete
 */
void UltraDMXProDevice::Configure(RpcController *controller,
                                  const string &request,
                                  string *response,
                                  ConfigureCallback *done) {
  Request request_pb;
  if (!request_pb.ParseFromString(request)) {
    controller->SetFailed("Invalid Request");
    done->Run();
    return;
  }

  switch (request_pb.type()) {
    case ola::plugin::usbpro::Request::USBPRO_PARAMETER_REQUEST:
      HandleParametersRequest(controller, &request_pb, response, done);
      break;
    case ola::plugin::usbpro::Request::USBPRO_SERIAL_REQUEST:
      HandleSerialRequest(controller, &request_pb, response, done);
      break;
    default:
      controller->SetFailed("Invalid Request");
      done->Run();
  }
}


/**
 * Update the cached param values
 */
void UltraDMXProDevice::UpdateParams(bool status,
                                     const usb_pro_parameters &params) {
  if (status) {
    m_got_parameters = true;
    m_break_time = params.break_time;
    m_mab_time = params.mab_time;
    m_rate = params.rate;
  }
}


/*
 * Handle a parameter request. This may set some parameters in the widget.
 * If no parameters are set we simply fetch the parameters and return them to
 * the client. If we are setting parameters, we send a SetParam() request and
 * then another GetParam() request in order to return the latest values to the
 * client.
 */
void UltraDMXProDevice::HandleParametersRequest(RpcController *controller,
                                                const Request *request,
                                                string *response,
                                                ConfigureCallback *done) {
  if (request->has_parameters() &&
      (request->parameters().has_break_time() ||
       request->parameters().has_mab_time() ||
       request->parameters().has_rate())) {
    if (!m_got_parameters) {
      controller->SetFailed("SetParameters failed, startup not complete");
      done->Run();
      return;
    }

    bool ret = m_ultra_widget->SetParameters(
      request->parameters().has_break_time() ?
        request->parameters().break_time() : m_break_time,
      request->parameters().has_mab_time() ?
        request->parameters().mab_time() : m_mab_time,
      request->parameters().has_rate() ?
        request->parameters().rate() : m_rate);

    if (!ret) {
      controller->SetFailed("SetParameters failed");
      done->Run();
      return;
    }
  }

  m_ultra_widget->GetParameters(NewSingleCallback(
    this,
    &UltraDMXProDevice::HandleParametersResponse,
    controller,
    response,
    done));
}


/**
 * Handle the GetParameters response
 */
void UltraDMXProDevice::HandleParametersResponse(
    RpcController *controller,
    string *response,
    ConfigureCallback *done,
    bool status,
    const usb_pro_parameters &params) {
  if (!status) {
    controller->SetFailed("GetParameters failed");
  } else {
    UpdateParams(true, params);
    Reply reply;
    reply.set_type(ola::plugin::usbpro::Reply::USBPRO_PARAMETER_REPLY);
    ola::plugin::usbpro::ParameterReply *parameters_reply =
      reply.mutable_parameters();

    parameters_reply->set_firmware_high(params.firmware_high);
    parameters_reply->set_firmware(params.firmware);
    parameters_reply->set_break_time(params.break_time);
    parameters_reply->set_mab_time(params.mab_time);
    parameters_reply->set_rate(params.rate);
    reply.SerializeToString(response);
  }
  done->Run();
}


/*
 * Handle a Serial number Configure RPC. We can just return the cached number.
 */
void UltraDMXProDevice::HandleSerialRequest(
    RpcController *controller,
    const Request *request,
    string *response,
    ConfigureCallback *done) {
  Reply reply;
  reply.set_type(ola::plugin::usbpro::Reply::USBPRO_SERIAL_REPLY);
  ola::plugin::usbpro::SerialNumberReply *serial_reply =
    reply.mutable_serial_number();
  serial_reply->set_serial(m_serial);
  reply.SerializeToString(response);
  done->Run();
  (void) controller;
  (void) request;
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
