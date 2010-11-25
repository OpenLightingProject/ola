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

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/service.h>
#include <iomanip>
#include <iostream>
#include <string>

#include "ola/BaseTypes.h"
#include "ola/Closure.h"
#include "ola/Logging.h"
#include "olad/PortDecorators.h"
#include "olad/Preferences.h"
#include "plugins/usbpro/UsbProDevice.h"
#include "plugins/usbpro/WidgetDetector.h"

namespace ola {
namespace plugin {
namespace usbpro {

using google::protobuf::RpcController;
using ola::plugin::usbpro::Request;
using ola::plugin::usbpro::Reply;

/*
 * Create a new device
 * @param owner  the plugin that owns this device
 * @param name  the device name
 * @param dev_path  path to the pro widget
 */
UsbProDevice::UsbProDevice(ola::PluginAdaptor *plugin_adaptor,
                           ola::AbstractPlugin *owner,
                           const string &name,
                           UsbWidget *widget,
                           uint16_t esta_id,
                           uint16_t device_id,
                           uint32_t serial,
                           unsigned int fps_limit):
    UsbDevice(owner, name, widget),
    m_got_parameters(false),
    m_in_shutdown(false),
    m_serial() {
  m_widget->SetMessageHandler(
      NewCallback(this, &UsbProDevice::HandleMessage));

  std::stringstream str;
  str << std::setfill('0');
  uint8_t *ptr = reinterpret_cast<uint8_t*>(&serial);
  for (int i = DeviceInformation::SERIAL_LENGTH - 1; i >= 0; i--) {
    int digit = (10 * (ptr[i] & 0xf0) >> 4) + (ptr[i] & 0x0f);
    str <<  std::setw(2)  << digit;
  }
  m_serial = str.str();

  GetParameters();  // update our local values

  UsbProInputPort *input_port = new UsbProInputPort(
      this,
      0,
      plugin_adaptor->WakeUpTime(),
      "");
  AddPort(input_port);
  OutputPort *output_port = new ThrottledOutputPortDecorator(
      new UsbProOutputPort(this, 0, ""),
      plugin_adaptor->WakeUpTime(),
      10,  // start with 10 tokens in the bucket
      fps_limit);  // 200 frames per second seems to be the limit

  AddPort(output_port);
  Start();  // this does nothing but set IsEnabled() to true
  (void) esta_id;
  (void) device_id;
}


/*
 * Handle Messages from the widget
 */
void UsbProDevice::HandleMessage(uint8_t label,
                                 unsigned int length,
                                 const uint8_t *data) {
  switch (label) {
    case REPROGRAM_FIRMWARE_LABEL:
      break;
    case PARAMETERS_LABEL:
      HandleParameters(data, length);
      break;
    case RECEIVED_DMX_LABEL:
      HandleDMX(data, length);
      break;
    case DMX_CHANGED_LABEL:
      HandleDMXDiff(data, length);
      break;
    case UsbWidget::SERIAL_LABEL:
      break;
    default:
      OLA_WARN << "Unknown message type " << label;
  }
}


/*
 * Stop this device
 */
void UsbProDevice::PrePortStop() {
  m_in_shutdown = true;  // don't allow any more writes
}


/*
 * Send the dmx out the widget
 * @return true on success, false on failure
 */
bool UsbProDevice::SendDMX(const DmxBuffer &buffer) {
  struct {
    uint8_t start_code;
    uint8_t dmx[DMX_UNIVERSE_SIZE];
  } widget_dmx;

  if (!IsEnabled())
    return true;

  widget_dmx.start_code = 0;
  unsigned int length = DMX_UNIVERSE_SIZE;
  buffer.Get(widget_dmx.dmx, &length);
  return m_widget->SendMessage(UsbWidget::DMX_LABEL,
                               length + 1,
                               reinterpret_cast<uint8_t*>(&widget_dmx));
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
                             google::protobuf::Closure *done) {
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


/*
 * Put the device back into recv mode
 * @return true on success, false on failure
 */
bool UsbProDevice::ChangeToReceiveMode(bool change_only) {
  if (m_in_shutdown)
    return true;

  uint8_t mode = change_only;
  bool status = m_widget->SendMessage(DMX_RX_MODE_LABEL, sizeof(mode), &mode);

  if (status && change_only)
    m_input_buffer.Blackout();
  return status;
}



/*
 * Called when we get new parameters from the widget.
 */
void UsbProDevice::HandleParameters(const uint8_t *data, unsigned int length) {
  if (m_outstanding_param_requests.empty())
    return;

  // parameters
  typedef struct {
    uint8_t firmware;
    uint8_t firmware_high;
    uint8_t break_time;
    uint8_t mab_time;
    uint8_t rate;
  } widget_parameters_reply;

  if (length < sizeof(widget_parameters_reply))
    return;

  const widget_parameters_reply *widget_reply =
    reinterpret_cast<const widget_parameters_reply*>(data);

  // update local values
  m_got_parameters = true;
  m_break_time = widget_reply->break_time;
  m_mab_time = widget_reply->mab_time;
  m_rate = widget_reply->rate;

  if (!m_outstanding_param_requests.size())
    return;

  OutstandingParamRequest parameter_request =
    m_outstanding_param_requests.front();
  m_outstanding_param_requests.pop_front();

  Reply reply;
  reply.set_type(ola::plugin::usbpro::Reply::USBPRO_PARAMETER_REPLY);
  ola::plugin::usbpro::ParameterReply *parameters_reply =
    reply.mutable_parameters();

  parameters_reply->set_firmware_high(widget_reply->firmware_high);
  parameters_reply->set_firmware(widget_reply->firmware);
  parameters_reply->set_break_time(widget_reply->break_time);
  parameters_reply->set_mab_time(widget_reply->mab_time);
  parameters_reply->set_rate(widget_reply->rate);
  reply.SerializeToString(parameter_request.response);
  parameter_request.closure->Run();
}


/*
 * Handle the dmx frame
 */
void UsbProDevice::HandleDMX(const uint8_t *data, unsigned int length) {
  typedef struct {
    uint8_t status;
    uint8_t dmx[DMX_UNIVERSE_SIZE + 1];
  } widget_dmx;

  if (length < 2)
    return;

  const widget_dmx *widget_reply =
    reinterpret_cast<const widget_dmx*>(data);

  if (widget_reply->status) {
    OLA_WARN << "UsbPro got corrupted packet, status: " <<
      static_cast<int>(widget_reply->status);
    return;
  }

  // only handle start code = 0
  if (length > 2 && widget_reply->dmx[0] == 0) {
    m_input_buffer.Set(widget_reply->dmx + 1, length - 2);
    InputPort *port = GetInputPort(0);
    port->DmxChanged();
  }
  return;
}


/*
 * Handle the dmx change of state frame
 */
void UsbProDevice::HandleDMXDiff(const uint8_t *data, unsigned int length) {
  typedef struct {
    uint8_t start;
    uint8_t changed[5];
    uint8_t data[40];
  } widget_data_changed;

  if (length < sizeof(widget_data_changed))
    return;

  const widget_data_changed *widget_reply =
    reinterpret_cast<const widget_data_changed*>(data);

  unsigned int start_channel = widget_reply->start * 8;
  unsigned int offset = 0;

  // skip non-0 start codes, this code is pretty messed up because the USB Pro
  // doesn't seem to provide a guarentee on the ordering of packets. Packets
  // with non-0 start codes are almost certainly going to cause problems.
  if (start_channel == 0 && (widget_reply->changed[0] & 0x01) &&
      widget_reply->data[offset])
    return;

  for (int i = 0; i < 40; i++) {
    if (start_channel + i > DMX_UNIVERSE_SIZE + 1 || offset + 6 >= length)
      break;

    if (widget_reply->changed[i/8] & (1 << (i % 8)) && start_channel + i != 0) {
      m_input_buffer.SetChannel(start_channel + i - 1,
                                widget_reply->data[offset]);
      offset++;
    }
  }

  InputPort *port = GetInputPort(0);
  port->DmxChanged();
  return;
}


/*
 * Fetch the widget parameters.
 * @returns true if we sent ok, false otherwise
 */
bool UsbProDevice::GetParameters() const {
  uint16_t user_size = 0;
  return m_widget->SendMessage(PARAMETERS_LABEL,
                               sizeof(user_size),
                               reinterpret_cast<uint8_t*>(&user_size));
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
                                           google::protobuf::Closure *done) {
  struct {
    uint16_t length;
    uint8_t break_time;
    uint8_t mab_time;
    uint8_t rate;
  } widget_parameters;

  if (request->has_parameters() &&
      (request->parameters().has_break_time() ||
       request->parameters().has_mab_time() ||
       request->parameters().has_rate())) {
    if (!m_got_parameters) {
      controller->SetFailed("SetParameters failed, startup not complete");
      done->Run();
      return;
    }

    widget_parameters.length = 0;
    widget_parameters.break_time = request->parameters().has_break_time() ?
      request->parameters().break_time() : m_break_time;
    widget_parameters.mab_time = request->parameters().has_mab_time() ?
      request->parameters().mab_time() : m_mab_time;
    widget_parameters.rate = request->parameters().has_rate() ?
      request->parameters().rate() : m_rate;

    bool ret = m_widget->SendMessage(
        SET_PARAMETERS_LABEL,
        sizeof(widget_parameters),
        reinterpret_cast<uint8_t*>(&widget_parameters));

    if (!ret) {
      controller->SetFailed("SetParameters failed");
      done->Run();
      return;
    }
  }

  if (!GetParameters()) {
    controller->SetFailed("GetParameters failed");
    done->Run();
  } else {
    // TODO(simonn): we should time these out if we don't get a response
    OutstandingParamRequest pending_request;
    pending_request.controller = controller;
    pending_request.response = response;
    pending_request.closure = done;
    m_outstanding_param_requests.push_back(pending_request);
  }
}


/*
 * Handle a Serial number Configure RPC. We can just return the cached number.
 */
void UsbProDevice::HandleSerialRequest(
    RpcController *controller,
    const Request *request,
    string *response,
    google::protobuf::Closure *done) {

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
}  // usbpro
}  // plugin
}  // ola
