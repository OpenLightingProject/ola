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
 *  lla-dev-info.cpp
 *  Displays the available devices and ports
 *  Copyright (C) 2005-2009 Simon Newton
 */

#include <errno.h>
#include <getopt.h>
#include <iostream>
#include <string>

#include <lla/plugin_id.h>
#include <lla/usbpro/ConfigMessages.pb.h>
#include "LlaConfigurator.h"

using namespace std;

/*
 * the config_mode is determined by the name in which we were called
 */
typedef enum {
  NONE,
  MODE_GET_PARAM,
  MODE_GET_SERIAL,
  MODE_SET_PARAM
} config_mode;


typedef struct {
  config_mode mode; // config_mode
  string command;   // argv[0]
  int device_id;    // device id
  bool help;        // help
  bool verbose;     // verbose
  int brk;          // brk
  int mab;          // mab
  int rate;         // rate
} options;


/*
 * A class which configures UsbPro devices.
 */
class UsbProConfigurator: public LlaConfigurator {
  public:
    UsbProConfigurator(options &opts):
      LlaConfigurator(opts.device_id, LLA_PLUGIN_USBPRO),
      m_opts(opts) {}
    void HandleConfigResponse(const string &reply, const string &error);
    void SendConfigRequest();
    bool SendGetParameters();
    bool SendGetSerial();
    bool SendSetParameters();
  private:
    void DisplayParameters(const lla::plugin::usbpro::ParameterReply &reply);
    void DisplaySerial(const lla::plugin::usbpro::SerialNumberReply &reply);
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

  lla::plugin::usbpro::Reply reply_pb;
  if (!reply_pb.ParseFromString(reply)) {
    cout << "Protobuf parsing failed" << endl;
    return;
  }
  if (reply_pb.type() == lla::plugin::usbpro::Reply::USBPRO_PARAMETER_REPLY &&
      reply_pb.has_parameters()) {
    DisplayParameters(reply_pb.parameters());
    return;
  } else if (reply_pb.type() == lla::plugin::usbpro::Reply::USBPRO_SERIAL_REPLY
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
    case MODE_GET_PARAM:
      SendGetParameters();
      break;
    case MODE_GET_SERIAL:
      SendGetSerial();
      break;
    case MODE_SET_PARAM:
      SendSetParameters();
      break;
    default:
      cout << "Unknown mode" << endl;
      Terminate();
  }
}


/*
 * Send a get parameters request
 */
bool UsbProConfigurator::SendGetParameters() {
  lla::plugin::usbpro::Request request;
  request.set_type(lla::plugin::usbpro::Request::USBPRO_PARAMETER_REQUEST);
  return SendMessage(request);
}


/*
 * Send a get serial request
 */
bool UsbProConfigurator::SendGetSerial() {
  lla::plugin::usbpro::Request request;
  request.set_type(lla::plugin::usbpro::Request::USBPRO_SERIAL_REQUEST);
  return SendMessage(request);
}


/*
 * Send a set param request
 */
bool UsbProConfigurator::SendSetParameters() {
  lla::plugin::usbpro::Request request;
  lla::plugin::usbpro::SetParameterRequest *set_request =
    request.mutable_set_parameters();
  request.set_type(lla::plugin::usbpro::Request::USBPRO_SET_PARAMETER_REQUEST);
  if (m_opts.brk != -1)
    set_request->set_break_time(m_opts.brk);
  if (m_opts.mab != -1)
    set_request->set_mab_time(m_opts.mab);
  if (m_opts.rate != -1)
    set_request->set_rate(m_opts.rate);
  return SendMessage(request);
}


/*
 * Display the widget parameters
 */
void UsbProConfigurator::DisplayParameters(
    const lla::plugin::usbpro::ParameterReply &reply) {
  /*
  uint16_t firm = rep->get_firmware();
  printf("Device %i\n", dev);
  printf(" Firmware: %hhi.%hhi\n", (firm & 0xFF00) >> 8 , firm & 0x00FF);
  printf(" Break Time: %0.2f us\n", rep->get_brk() * 10.67);
  printf(" MAB Time: %0.2f us\n", rep->get_mab() * 10.67);
  printf(" Packet Rate: %i packets/sec\n", rep->get_rate());
  */
  cout << "in displ param" << endl;
}


/*
 * Display the serial number
 */
void UsbProConfigurator::DisplaySerial(
    const lla::plugin::usbpro::SerialNumberReply &reply) {
  /*
  uint8_t ser[UsbProConfMsgSerRep::SERIAL_SIZE];

  rep->get_serial(ser, UsbProConfMsgSerRep::SERIAL_SIZE);
  printf("Device %i\n", dev);
  printf(" Serial ");
  for (int i=3; i>=0; i--) {
    printf("%i%i", (ser[i] & 0xf0) >> 4, ser[i] & 0x0f);
  }
  printf("\n");
  */
  cout << reply.serial() << endl;
}


/*
 * This is called when we recieve a device config response
 * @param dev the device id
 * @param res the response buffer
 * @param len the length of the response buffer
int Observer::dev_config(unsigned int dev, uint8_t *rep, unsigned int len) {
  UsbProConfMsg *m = m_parser->parse(rep, len);
  struct timespec tv;

  if( m == NULL) {
    printf("bad response\n");
    return 0;
  }

  switch(m->type()) {

    case LLA_USBPRO_CONF_MSG_PRM_REP:
      handle_params(dev, (UsbProConfMsgPrmRep*) m);
      m_term = 1;
      break;
    case LLA_USBPRO_CONF_MSG_SER_REP:
      handle_serial(dev, (UsbProConfMsgSerRep*) m);
      m_term = 1;
      break;
    case LLA_USBPRO_CONF_MSG_SPRM_REP:
      m_opts->m = MODE_GET_PARAM;
      tv.tv_sec = 0;
      tv.tv_nsec = 30000000;
      // wait for the widget to update
      nanosleep(&tv, NULL);
      get_params();
      break;
    default:
      delete m;
      return 0;
  }
  delete m;

  return 0;
}
 */


/*
 * Init options
 */
void InitOptions(options &opts) {
  opts.mode = NONE;
  opts.device_id = -1;
  opts.help = false;
  opts.verbose = false;
  opts.brk = -1;
  opts.mab = -1;
  opts.rate = -1;
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
      {"params",  required_argument,  0, 'p'},
      {"rate",    required_argument,  0, 'r'},
      {"serial",  required_argument,  0, 's'},
      {"verbose", no_argument,        0, 'v'},
      {0, 0, 0, 0}
    };

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "b:d:hm:pr:sv", long_options, &option_index);
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
      case 'p':
        opts.mode = MODE_GET_PARAM;
        break;
      case 'r':
        opts.rate = atoi(optarg);
        break;
      case 's':
        opts.mode = MODE_GET_SERIAL;
        break;
      case 'v':
        opts.verbose = true;
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
    "-d <dev_id> [--params | --serial | -b <brk> -m <mab> -r <rate> ]\n\n"
    "Configure Enttec Usb Pro Devices managed by LLA.\n\n"
    "  -b, --brk <brk>     Set the break time (9 - 127)\n"
    "  -d, --dev <dev_id>  The device id to configure\n"
    "  -h, --help          Display this help message and exit.\n"
    "  -m, --mab <mab>     Set the make after-break-time (1 - 127)\n"
    "  -p, --params        Get the parameters.\n"
    "  -r, --rate <rate>   Set the transmission rate (1 - 40).\n"
    "  -s, --serial         Get the serial number.\n"
    "  -v, --verbose       Display this message.\n" <<
    endl;
  exit(0);
}


void CheckOptions(options &opts) {
  // check for valid parameters
  if(opts.brk != -1 || opts.mab != -1 || opts.rate != -1)
    opts.mode = MODE_SET_PARAM;

  if (opts.brk != -1 && (opts.brk < 9 || opts.brk > 127))
    opts.mode = NONE;

  if (opts.mab != -1 && (opts.mab < 1 || opts.mab > 127))
    opts.mode = NONE;

  if (opts.rate != -1 && (opts.rate < 1 || opts.rate > 40))
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
