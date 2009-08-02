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
 *  ola-dev-info.cpp
 *  Displays the available devices and ports
 *  Copyright (C) 2005-2009 Simon Newton
 */

#include <errno.h>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <string>

#include <ola/plugin_id.h>
#include <ola/usbpro/UsbProConfigMessages.pb.h>
#include "OlaConfigurator.h"

using namespace std;

static const int K_INVALID_VALUE = -1;

/*
 * the config_mode is determined by the name in which we were called
 */
typedef enum {
  NONE,
  MODE_PARAM,
  MODE_SERIAL,
} config_mode;


typedef struct {
  config_mode mode; // config_mode
  string command;   // argv[0]
  int device_id;    // device id
  bool help;        // help
  int brk;          // brk
  int mab;          // mab
  int rate;         // rate
} options;


/*
 * A class which configures UsbPro devices.
 */
class UsbProConfigurator: public OlaConfigurator {
  public:
    UsbProConfigurator(options &opts):
      OlaConfigurator(opts.device_id, ola::OLA_PLUGIN_USBPRO),
      m_opts(opts) {}
    void HandleConfigResponse(const string &reply, const string &error);
    void SendConfigRequest();
    bool SendParametersRequest();
    bool SendSerialRequest();
  private:
    void DisplayParameters(const ola::plugin::usbpro::ParameterReply &reply);
    void DisplaySerial(const ola::plugin::usbpro::SerialNumberReply &reply);
    options m_opts;
};


/*
 * Handle the device config reply
 */
void UsbProConfigurator::HandleConfigResponse(const string &reply,
                                              const string &error) {
  Terminate();
  if (!error.empty()) {
    cout << error << endl;
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
  }
  cout << "Invalid response type or missing options field" << endl;
}


/*
 * Send the appropriate ConfigRequest
 */
void UsbProConfigurator::SendConfigRequest() {
  switch (m_opts.mode) {
    case MODE_PARAM:
      SendParametersRequest();
      break;
    case MODE_SERIAL:
      SendSerialRequest();
      break;
    default:
      cout << "Unknown mode" << endl;
      Terminate();
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
  if (m_opts.brk != K_INVALID_VALUE)
    parameter_request->set_break_time(m_opts.brk);
  if (m_opts.mab != K_INVALID_VALUE)
    parameter_request->set_mab_time(m_opts.mab);
  if (m_opts.rate != K_INVALID_VALUE)
    parameter_request->set_rate(m_opts.rate);
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
 * Display the widget parameters
 */
void UsbProConfigurator::DisplayParameters(
    const ola::plugin::usbpro::ParameterReply &reply) {

  cout << "Device: " << m_alias << endl;
  cout << "Firmware: " << reply.firmware_high() << "." << reply.firmware() <<
    endl;
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
 * Init options
 */
void InitOptions(options &opts) {
  opts.mode = MODE_PARAM;
  opts.device_id = K_INVALID_VALUE;
  opts.help = false;
  opts.brk = K_INVALID_VALUE;
  opts.mab = K_INVALID_VALUE;
  opts.rate = K_INVALID_VALUE;
}


/*
 * Parse our cmd line options
 */
int ParseOptions(int argc, char *argv[], options &opts) {
  static struct option long_options[] = {
      {"brk",     required_argument,  0, 'k'},
      {"dev",     required_argument,  0, 'd'},
      {"help",    no_argument,        0, 'h'},
      {"mab",     required_argument,  0, 'm'},
      {"rate",    required_argument,  0, 'r'},
      {"serial",  required_argument,  0, 's'},
      {0, 0, 0, 0}
    };

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "b:d:hm:r:s", long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'b':
        opts.brk = atoi(optarg);
        break;
      case 'd':
        opts.device_id = atoi(optarg);
        break;
      case 'h':
        opts.help = true;
        break;
      case 'm':
        opts.mab = atoi(optarg);
        break;
      case 'r':
        opts.rate = atoi(optarg);
        break;
      case 's':
        opts.mode = MODE_SERIAL;
        break;
      case '?':
        break;
    }
  }
  return 0;
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(options &opts) {
  cout << "Usage: " << opts.command <<
    "-d <dev_id> [ --serial | -b <brk> -m <mab> -r <rate> ]\n\n"
    "Configure Enttec Usb Pro Devices managed by OLA.\n\n"
    "  -b, --brk <brk>     Set the break time (9 - 127)\n"
    "  -d, --dev <device>  The device to configure\n"
    "  -h, --help          Display this help message and exit.\n"
    "  -m, --mab <mab>     Set the make after-break-time (1 - 127)\n"
    "  -r, --rate <rate>   Set the transmission rate (1 - 40).\n"
    "  -s, --serial        Get the serial number.\n" <<
    endl;
  exit(0);
}


void CheckOptions(options &opts) {
  // check for valid parameters
  if (opts.brk != K_INVALID_VALUE && (opts.brk < 9 || opts.brk > 127))
    opts.mode = NONE;

  if (opts.mab != K_INVALID_VALUE && (opts.mab < 1 || opts.mab > 127))
    opts.mode = NONE;

  if (opts.rate != K_INVALID_VALUE && (opts.rate < 1 || opts.rate > 40))
    opts.mode = NONE;
}


/*
 * The main function
 */
int main(int argc, char *argv[]) {
  options opts;
  opts.command = argv[0];
  InitOptions(opts);
  ParseOptions(argc, argv, opts);
  CheckOptions(opts);

  if (opts.help || opts.device_id < 0 || opts.mode == NONE)
    DisplayHelpAndExit(opts);

  UsbProConfigurator configurator(opts);
  if (!configurator.Setup()) {
    cout << "error: " << strerror(errno) << endl;
    exit(1);
  }

  configurator.Run();
  return 0;
}
