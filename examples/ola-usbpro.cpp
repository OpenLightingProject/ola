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
 * ola-usbpro.cpp
 * Configure Enttec USB Pro Devices managed by OLA
 * Copyright (C) 2005 Simon Newton
 */

#include <errno.h>
#include <ola/plugin_id.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <plugins/usbpro/messages/UsbProConfigMessages.pb.h>
#include <iomanip>
#include <iostream>
#include <string>
#include "examples/OlaConfigurator.h"


using std::cerr;
using std::cout;
using std::endl;
using std::string;

DECLARE_int32(device);
DEFINE_s_default_bool(assignments, a, false, "Get the port assignments.");
DEFINE_s_int32(brk, b, -1, "Set the break time (9 - 127).");
DEFINE_s_default_bool(get_params, g, false, "Get the current parameters.");
DEFINE_s_int32(mab, m, -1, "Set the make after-break-time (1 - 127).");
DEFINE_s_int32(port, p, -1, "The port to configure.");
DEFINE_s_int32(rate, r, -1, "Set the transmission rate (1 - 40).");
DEFINE_s_default_bool(serial, s, false, "Get the serial number.");

/*
 * A class which configures UsbPro devices.
 */
class UsbProConfigurator: public OlaConfigurator {
 public:
    UsbProConfigurator()
        : OlaConfigurator(FLAGS_device, ola::OLA_PLUGIN_USBPRO) {}
    void HandleConfigResponse(const string &reply, const string &error);
    void SendConfigRequest();
    bool SendParametersRequest();
    bool SendSerialRequest();
    bool SendPortAssignmentRequest();

 private:
    void DisplayParameters(const ola::plugin::usbpro::ParameterReply &reply);
    void DisplaySerial(const ola::plugin::usbpro::SerialNumberReply &reply);
    void DisplayPortAssignment(
        const ola::plugin::usbpro::PortAssignmentReply &reply);
};


/*
 * Handle the device config reply
 */
void UsbProConfigurator::HandleConfigResponse(const string &reply,
                                              const string &error) {
  Terminate();
  if (!error.empty()) {
    cerr << error << endl;
    return;
  }

  ola::plugin::usbpro::Reply reply_pb;
  if (!reply_pb.ParseFromString(reply)) {
    cout << "Protobuf parsing failed" << endl;
    return;
  }
  if (reply_pb.type() == ola::plugin::usbpro::Reply::USBPRO_PARAMETER_REPLY &&
      reply_pb.has_parameters()) {
    DisplayParameters(reply_pb.parameters());
    return;
  } else if (reply_pb.type() == ola::plugin::usbpro::Reply::USBPRO_SERIAL_REPLY
             && reply_pb.has_serial_number()) {
    DisplaySerial(reply_pb.serial_number());
    return;
  } else if (reply_pb.type() ==
             ola::plugin::usbpro::Reply::USBPRO_PORT_ASSIGNMENT_REPLY
             && reply_pb.has_port_assignment()) {
    DisplayPortAssignment(reply_pb.port_assignment());
    return;
  }
  cout << "Invalid response type or missing options field" << endl;
}


/*
 * Send the appropriate ConfigRequest
 */
void UsbProConfigurator::SendConfigRequest() {
  if (FLAGS_serial) {
    SendSerialRequest();
  } else if (FLAGS_assignments) {
    SendPortAssignmentRequest();
  } else {
    // Also FLAGS_get_params
    SendParametersRequest();
  }
}


/*
 * Send a get parameters request
 */
bool UsbProConfigurator::SendParametersRequest() {
  ola::plugin::usbpro::Request request;
  request.set_type(ola::plugin::usbpro::Request::USBPRO_PARAMETER_REQUEST);

  ola::plugin::usbpro::ParameterRequest *parameter_request =
    request.mutable_parameters();
  parameter_request->set_port_id(FLAGS_port);
  if (FLAGS_brk.present()) {
    parameter_request->set_break_time(FLAGS_brk);
  }
  if (FLAGS_mab.present()) {
    parameter_request->set_mab_time(FLAGS_mab);
  }
  if (FLAGS_rate.present()) {
    parameter_request->set_rate(FLAGS_rate);
  }
  return SendMessage(request);
}


/*
 * Send a get serial request
 */
bool UsbProConfigurator::SendSerialRequest() {
  ola::plugin::usbpro::Request request;
  request.set_type(ola::plugin::usbpro::Request::USBPRO_SERIAL_REQUEST);
  return SendMessage(request);
}


/*
 * Send a get port assignment request
 */
bool UsbProConfigurator::SendPortAssignmentRequest() {
  ola::plugin::usbpro::Request request;
  request.set_type(
      ola::plugin::usbpro::Request::USBPRO_PORT_ASSIGNMENT_REQUEST);
  return SendMessage(request);
}


/*
 * Display the widget parameters
 */
void UsbProConfigurator::DisplayParameters(
    const ola::plugin::usbpro::ParameterReply &reply) {

  cout << "Device: " << m_alias << endl;
  cout << "Firmware: " << reply.firmware_high() << "." << reply.firmware()
       << endl;
  cout << "Break Time: " << reply.break_time() * 10.67 << "us" << endl;
  cout << "MAB Time: " <<  reply.mab_time() * 10.67 << "us" << endl;
  cout << "Packet Rate: " << reply.rate() << " packets/sec" << endl;
}


/*
 * Display the serial number
 */
void UsbProConfigurator::DisplaySerial(
    const ola::plugin::usbpro::SerialNumberReply &reply) {

  string serial_number = reply.serial();
  cout << "Device: " << m_alias << endl;
  cout << "Serial: " << reply.serial() << endl;
}


/*
 * Display the port assignments
 */
void UsbProConfigurator::DisplayPortAssignment(
    const ola::plugin::usbpro::PortAssignmentReply &reply) {
  cout << "Device: " << m_alias << endl;
  cout << "Port 1: " << reply.port_assignment1() << endl;
  cout << "Port 2: " << reply.port_assignment2() << endl;
}


/*
 * The main function
 */
int main(int argc, char *argv[]) {
  ola::AppInit(
      &argc,
      argv,
      "-d <dev_id> [--serial | -p <port> --g | -p <port> -b <brk> -m <mab> -r "
          "<rate>]",
      "Configure Enttec USB Pro Devices managed by OLA.");

  if (FLAGS_device < 0) {
    ola::DisplayUsageAndExit();
  }

  // check for valid parameters
  if (FLAGS_brk.present() && (FLAGS_brk < 9 || FLAGS_brk > 127)) {
    ola::DisplayUsageAndExit();
  }

  if (FLAGS_mab.present() && (FLAGS_mab < 1 || FLAGS_mab > 127)) {
    ola::DisplayUsageAndExit();
  }

  if (FLAGS_rate.present() && (FLAGS_rate < 1 || FLAGS_rate > 40)) {
    ola::DisplayUsageAndExit();
  }

  if ((FLAGS_get_params || (!FLAGS_assignments && !FLAGS_serial)) &&
      (FLAGS_port < 0)) {
    ola::DisplayUsageAndExit();
  }

  UsbProConfigurator configurator;
  if (!configurator.Setup()) {
    cerr << "Error: " << strerror(errno) << endl;
    exit(1);
  }

  configurator.Run();
  return 0;
}
