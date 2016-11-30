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
 * UsbProDevice.cpp
 * An Enttec Usb Pro device
 * Copyright (C) 2006 Simon Newton
 *
 * The device creates two ports, one in and one out, but you can only use one
 * at a time.
 */

#include <iomanip>
#include <iostream>
#include <string>

#include "common/rpc/RpcController.h"
#include "ola/Callback.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "olad/Preferences.h"
#include "plugins/usbpro/UsbProDevice.h"
#include "plugins/usbpro/UsbProWidgetDetector.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::plugin::usbpro::Reply;
using ola::plugin::usbpro::Request;
using ola::rpc::RpcController;
using std::ostringstream;
using std::string;

/*
 * Create a new device
 * @param owner  the plugin that owns this device
 * @param name  the device name
 * @param dev_path  path to the pro widget
 */
UsbProDevice::UsbProDevice(ola::PluginAdaptor *plugin_adaptor,
                           ola::AbstractPlugin *owner,
                           const string &name,
                           EnttecUsbProWidget *widget,
                           uint32_t serial,
                           uint16_t firmware_version,
                           unsigned int fps_limit)
    : UsbSerialDevice(owner, name, widget),
      m_pro_widget(widget),
      m_serial(SerialToString(serial)) {
  ostringstream str;
  str << name << ", Serial #: " << m_serial << ", firmware "
      << (firmware_version >> 8) << "." << (firmware_version & 0xff);
  SetName(str.str());

  for (unsigned int i = 0; i < widget->PortCount(); i++) {
    EnttecPort *enttec_port = widget->GetPort(i);
    if (!enttec_port) {
      OLA_WARN << "GetPort() returned NULL";
      continue;
    }

    ostringstream port_description;
    if (widget->PortCount() > 1) {
      port_description << "DMX" << IntToString(i+1);
    }

    UsbProInputPort *input_port = new UsbProInputPort(
        this, enttec_port, i, plugin_adaptor, port_description.str());
    enttec_port->SetDMXCallback(
        NewCallback(static_cast<InputPort*>(input_port),
                    &InputPort::DmxChanged));
    AddPort(input_port);

    OutputPort *output_port = new UsbProOutputPort(
        this, enttec_port, i, port_description.str(),
        plugin_adaptor->WakeUpTime(),
        5,  // allow up to 5 burst frames
        fps_limit);  // 200 frames per second seems to be the limit
    AddPort(output_port);

    PortParams port_params = {false, 0, 0, 0};
    m_port_params.push_back(port_params);
    enttec_port->GetParameters(
      NewSingleCallback(this, &UsbProDevice::UpdateParams, i));
  }
}


/*
 * Stop this device
 */
void UsbProDevice::PrePortStop() {
  m_pro_widget->Stop();
}


/*
 * Handle device config messages
 * @param controller An RpcController
 * @param request the request data
 * @param response the response to return
 * @param done the closure to call once the request is complete
 */
void UsbProDevice::Configure(RpcController *controller,
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
    case ola::plugin::usbpro::Request::USBPRO_PORT_ASSIGNMENT_REQUEST:
      HandlePortAssignmentRequest(controller, &request_pb, response, done);
      break;
    default:
      controller->SetFailed("Invalid Request");
      done->Run();
  }
}


/**
 * Update the cached param values
 */
void UsbProDevice::UpdateParams(unsigned int port_id, bool status,
                                const usb_pro_parameters &params) {
  if (port_id >= m_port_params.size()) {
    return;
  }

  if (status) {
    PortParams &port_params = m_port_params[port_id];
    port_params.got_parameters = true;
    port_params.break_time = params.break_time;
    port_params.mab_time = params.mab_time;
    port_params.rate = params.rate;
  }
}


/*
 * Handle a parameter request. This may set some parameters in the widget.
 * If no parameters are set we simply fetch the parameters and return them to
 * the client. If we are setting parameters, we send a SetParam() request and
 * then another GetParam() request in order to return the latest values to the
 * client.
 */
void UsbProDevice::HandleParametersRequest(RpcController *controller,
                                           const Request *request,
                                           string *response,
                                           ConfigureCallback *done) {
  if (!request->has_parameters()) {
      controller->SetFailed("Invalid request");
      done->Run();
  }
  unsigned int port_id = request->parameters().port_id();

  EnttecPort *enttec_port = m_pro_widget->GetPort(port_id);
  if (enttec_port == NULL) {
      controller->SetFailed("Invalid port id");
      done->Run();
      return;
  }

  if (request->has_parameters() &&
      (request->parameters().has_break_time() ||
       request->parameters().has_mab_time() ||
       request->parameters().has_rate())) {
    PortParams &port_params = m_port_params[port_id];
    if (!port_params.got_parameters) {
      controller->SetFailed("SetParameters failed, startup not complete");
      done->Run();
      return;
    }

    bool ret = enttec_port->SetParameters(
      request->parameters().has_break_time() ?
        request->parameters().break_time() : port_params.break_time,
      request->parameters().has_mab_time() ?
        request->parameters().mab_time() : port_params.mab_time,
      request->parameters().has_rate() ?
        request->parameters().rate() : port_params.rate);

    if (!ret) {
      controller->SetFailed("SetParameters failed");
      done->Run();
      return;
    }
  }

  enttec_port->GetParameters(NewSingleCallback(
    this,
    &UsbProDevice::HandleParametersResponse,
    controller,
    response,
    done,
    port_id));
}


/**
 * Handle the GetParameters response
 */
void UsbProDevice::HandleParametersResponse(RpcController *controller,
                                            string *response,
                                            ConfigureCallback *done,
                                            unsigned int port_id,
                                            bool status,
                                            const usb_pro_parameters &params) {
  if (!status) {
    controller->SetFailed("GetParameters failed");
  } else {
    UpdateParams(port_id, true, params);
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
void UsbProDevice::HandleSerialRequest(RpcController*,
                                       const Request*,
                                       string *response,
                                       ConfigureCallback *done) {
  Reply reply;
  reply.set_type(ola::plugin::usbpro::Reply::USBPRO_SERIAL_REPLY);
  ola::plugin::usbpro::SerialNumberReply *serial_reply =
    reply.mutable_serial_number();
  serial_reply->set_serial(m_serial);
  reply.SerializeToString(response);
  done->Run();
}


/*
 * Handle a port assignment request.
 */
void UsbProDevice::HandlePortAssignmentRequest(RpcController *controller,
                                               const Request*,
                                               string *response,
                                               ConfigureCallback *done) {
  m_pro_widget->GetPortAssignments(NewSingleCallback(
    this,
    &UsbProDevice::HandlePortAssignmentResponse,
    controller,
    response,
    done));
}


/**
 * Handle a PortAssignment response.
 */
void UsbProDevice::HandlePortAssignmentResponse(RpcController *controller,
                                                string *response,
                                                ConfigureCallback *done,
                                                bool status,
                                                uint8_t port1_assignment,
                                                uint8_t port2_assignment) {
  if (!status) {
    controller->SetFailed("Get Port Assignments failed");
  } else {
    Reply reply;
    reply.set_type(ola::plugin::usbpro::Reply::USBPRO_PORT_ASSIGNMENT_REPLY);
    ola::plugin::usbpro::PortAssignmentReply *port_assignment_reply =
      reply.mutable_port_assignment();
    port_assignment_reply->set_port_assignment1(port1_assignment);
    port_assignment_reply->set_port_assignment2(port2_assignment);
    reply.SerializeToString(response);
  }
  done->Run();
}


string UsbProDevice::SerialToString(uint32_t serial) {
  ostringstream str;
  str << std::setfill('0');
  uint8_t *ptr = reinterpret_cast<uint8_t*>(&serial);
  for (int i = UsbProWidgetInformation::SERIAL_LENGTH - 1; i >= 0; i--) {
    int digit = (10 * (ptr[i] & 0xf0) >> 4) + (ptr[i] & 0x0f);
    str <<  std::setw(2)  << digit;
  }
  return str.str();
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
