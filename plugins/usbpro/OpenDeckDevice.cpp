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
 * OpenDeckDevice.cpp
 * An OpenDeck Device
 * Copyright (C) 2022 Igor Petrovic
 *
 * This device creates a single OpenDeck output port
 */

#include <iomanip>
#include <ios>
#include <iostream>
#include <string>

#include "common/rpc/RpcController.h"
#include "ola/Callback.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "plugins/usbpro/OpenDeckDevice.h"
#include "plugins/usbpro/UsbProWidgetDetector.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::plugin::usbpro::Request;
using ola::plugin::usbpro::Reply;
using ola::rpc::RpcController;
using ola::utils::SplitUInt16;
using std::string;

/*
 * Create a new device
 * @param owner  the plugin that owns this device
 * @param name  the device name
 * @param dev_path  path to the OpenDeck widget
 */
OpenDeckDevice::OpenDeckDevice(ola::PluginAdaptor *plugin_adaptor,
                               ola::AbstractPlugin *owner,
                               const string &name,
                               OpenDeckWidget *widget,
                               OLA_UNUSED uint16_t esta_id,
                               OLA_UNUSED uint16_t device_id,
                               uint32_t serial,
                               uint16_t firmware_version,
                               unsigned int fps_limit):
    UsbSerialDevice(owner, name, widget),
    m_opendeck_widget(widget),
    m_serial(),
    m_got_parameters(false),
    m_break_time(0),
    m_mab_time(0),
    m_rate(0) {
  std::ostringstream str;
  str << std::setfill('0');
  uint8_t *ptr = reinterpret_cast<uint8_t*>(&serial);
  for (int i = 0; i < UsbProWidgetInformation::SERIAL_LENGTH; i++) {
    str << std::setfill('0') << std::setw(2) << std::hex
        << static_cast<int>(ptr[i]);
  }

  m_serial = str.str();
  str.str("");

  // Firmware version is stored in major.minor format.
  // This scheme uses the following encoding:
  // MMMMMMMM mmmmmmmm
  // M: major
  // m: minor
  uint8_t major;
  uint8_t minor;

  SplitUInt16(firmware_version, &minor, &major);

  str << "Serial #: " << m_serial << ", firmware "
      << major << "." << minor;

  m_opendeck_widget->GetParameters(NewSingleCallback(
    this,
    &OpenDeckDevice::UpdateParams));

  // add the output port
  OutputPort *output_port = new OpenDeckOutputPort(
      this,
      m_opendeck_widget,
      0,
      str.str(),
      plugin_adaptor->WakeUpTime(),
      5,  // allow up to 5 burst frames
      fps_limit);
  AddPort(output_port);
}


/*
 * Stop this device
 */
void OpenDeckDevice::PrePortStop() {
  m_opendeck_widget->Stop();
}


/*
 * Handle device config messages
 * @param controller An RpcController
 * @param request the request data
 * @param response the response to return
 * @param done the closure to call once the request is complete
 */
void OpenDeckDevice::Configure(RpcController *controller,
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
void OpenDeckDevice::UpdateParams(bool status,
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
void OpenDeckDevice::HandleParametersRequest(RpcController *controller,
                                             const Request *request,
                                             string *response,
                                             ConfigureCallback *done) {
  if (request->has_parameters() &&
      (request->parameters().has_break_time() ||
       request->parameters().has_mab_time() ||
       request->parameters().has_rate())) {
    controller->SetFailed("SetParameters failed");
    done->Run();
    return;
  }

  m_opendeck_widget->GetParameters(NewSingleCallback(
    this,
    &OpenDeckDevice::HandleParametersResponse,
    controller,
    response,
    done));
}


/**
 * Handle the GetParameters response
 */
void OpenDeckDevice::HandleParametersResponse(
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
void OpenDeckDevice::HandleSerialRequest(
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
