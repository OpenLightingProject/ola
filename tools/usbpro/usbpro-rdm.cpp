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
 * usbpro-rdm.cpp
 * A very simple RDM sniffer for USB Pro like devices
 * Copyright (C) 2010 Simon Newton
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#include <sysexits.h>
#include <termios.h>
#include <ola/Logging.h>
#include <ola/Callback.h>
#include <ola/network/SelectServer.h>
#include <ola/rdm/RDMCommand.h>

#include <iostream>
#include <fstream>
#include <string>

#include "plugins/usbpro/UsbWidget.h"
#include "tools/usbpro/usbpro-common.h"

using std::cout;
using std::endl;
using std::string;
using ola::network::SelectServerInterface;
using ola::plugin::usbpro::UsbWidget;
using ola::rdm::RDMCommand;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;

static const char DEFAULT_DEVICE[] = "/dev/ttyUSB0";

typedef struct {
  bool help;
  bool dump_all;
  bool verbose;
  ola::log_level log_level;
  string device;
} options;


class RDMSniffer {
  public:
    RDMSniffer(UsbWidget *widget,
               SelectServerInterface *ss,
               bool dump_all,
               bool verbose);

    void HandleMessage(uint8_t label,
                       unsigned int length,
                       const uint8_t *data);

  private:
    UsbWidget *m_widget;
    SelectServerInterface *m_ss;
    bool m_dump_all;
    bool m_verbose;

    void DumpRawPacket(unsigned int length, const uint8_t *data);
    void DumpRequest(unsigned int length, const uint8_t *data);
    void DumpResponse(unsigned int length, const uint8_t *data);
    void DumpDiscover(unsigned int length, const uint8_t *data);

    static const uint8_t RECEIVED_DMX_LABEL = 5;
    static const uint8_t DRAFT_START_CODE = 0xf0;
};


RDMSniffer::RDMSniffer(UsbWidget *widget,
                       SelectServerInterface *ss,
                       bool dump_all,
                       bool verbose):
    m_widget(widget),
    m_ss(ss),
    m_dump_all(dump_all),
    m_verbose(verbose) {
  widget->SetMessageHandler(
      ola::NewCallback(this, &RDMSniffer::HandleMessage));
}


/*
 * Handle the widget replies
 */
void RDMSniffer::HandleMessage(uint8_t label,
                               unsigned int length,
                               const uint8_t *data) {
  if (label != RECEIVED_DMX_LABEL) {
    OLA_INFO << "Not a RECEIVED_DMX_LABEL, was " << static_cast<int>(label);
    return;
  }

  if (!length) {
    if (m_dump_all)
      OLA_INFO << "Zero sized packet";
    return;
  }

  if (data[0]) {
    OLA_INFO << "Error: " << static_cast<int>(data[0]);
    return;
  }

  length--;
  data++;

  if (!length) {
    if (m_dump_all)
      OLA_INFO << "Zero sized packet";
    return;
  }

  switch (data[0]) {
    case 0:
      if (m_dump_all)
        OLA_INFO << "DMX packet";
      return;
    case DRAFT_START_CODE:
      if (m_dump_all) {
        cout << "Draft RDM packet: ";
        DumpRawPacket(length, data);
      }
      return;
    case RDMCommand::START_CODE:
      break;
    default:
      if (m_dump_all)
        OLA_INFO << "Packet with start code " << static_cast<int>(data[0]);
      return;
  }

  if (length < 21) {
    cout << "Short packet: ";
    DumpRawPacket(length, data);
    return;
  }

  uint8_t command_class = data[20];
  cout << static_cast<int>(command_class) << endl;

  switch (command_class) {
    case RDMCommand::GET_COMMAND:
    case RDMCommand::SET_COMMAND:
      DumpRequest(length - 1, data + 1);
      return;
    case RDMCommand::GET_COMMAND_RESPONSE:
    case RDMCommand::SET_COMMAND_RESPONSE:
      DumpResponse(length - 1, data + 1);
      return;
    case RDMCommand::DISCOVER_COMMAND:
      DumpDiscover(length - 1, data + 1);
      return;
    default:
      DumpRawPacket(length -1, data + 1);
  }
}


void RDMSniffer::DumpRawPacket(unsigned int length, const uint8_t *data) {
  for (unsigned int i = 0; i < length; ++i)
    cout << std::hex << static_cast<int>(data[i]) << " ";
  cout << endl;
}


void RDMSniffer::DumpRequest(unsigned int length, const uint8_t *data) {
  RDMRequest *request = RDMRequest::InflateFromData(data, length);
  if (request) {
    cout << (request->CommandClass() == RDMCommand::GET_COMMAND ? "GET" :
             "SET");
    if (m_verbose)
      cout << " src: " << request->SourceUID() << ", dst: " <<
        request->DestinationUID() << ", transaction: " <<
        request->TransactionNumber() << ", port: " << request->PortId() <<
        ", PID " << std::hex << request->ParamId() << endl;
    else
      cout << " src: " << request->SourceUID() << ", dst: " <<
        request->DestinationUID() << ", PID " << std::hex <<
        request->ParamId() << endl;
  } else {
    DumpRawPacket(length, data);
  }
}


void RDMSniffer::DumpResponse(unsigned int length, const uint8_t *data) {
  RDMResponse *response = RDMResponse::InflateFromData(data, length);
  if (response) {
    cout << (response->CommandClass() == RDMCommand::GET_COMMAND_RESPONSE ?
        "GET_RESPONSE" : "SET_RESPONSE");
    if (m_verbose)
      cout << " src: " << response->SourceUID() << ", dst: " <<
        response->DestinationUID() << ", transaction: " <<
        response->TransactionNumber() << ", response type: " <<
        response->ResponseType() << ", PID " << std::hex << response->ParamId()
        << endl;
    else
      cout << " src: " << response->SourceUID() << ", dst: " <<
        response->DestinationUID() << ", PID " << std::hex <<
        response->ParamId() << endl;
  }
  DumpRawPacket(length, data);
}


void RDMSniffer::DumpDiscover(unsigned int length, const uint8_t *data) {
  DumpRawPacket(length, data);
}


/*
 * Parse our command line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"dump-all", no_argument, 0, 'a'},
      {"device", required_argument, 0, 'd'},
      {"help", no_argument, 0, 'h'},
      {"log-level", required_argument, 0, 'l'},
      {"verbose", no_argument, 0, 'v'},
      {0, 0, 0, 0}
    };

  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, "ad:hl:v", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'a':
        opts->dump_all = true;
        break;
      case 'd':
        opts->device = optarg;
        break;
      case 'h':
        opts->help = true;
        break;
      case 'l':
        switch (atoi(optarg)) {
          case 0:
            // nothing is written at this level
            // so this turns logging off
            opts->log_level = ola::OLA_LOG_NONE;
            break;
          case 1:
            opts->log_level = ola::OLA_LOG_FATAL;
            break;
          case 2:
            opts->log_level = ola::OLA_LOG_WARN;
            break;
          case 3:
            opts->log_level = ola::OLA_LOG_INFO;
            break;
          case 4:
            opts->log_level = ola::OLA_LOG_DEBUG;
            break;
          default :
            break;
        }
        break;
      case 'v':
        opts->verbose = true;
        break;
      case '?':
        break;
      default:
       break;
    }
  }
  return;
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(char *argv[]) {
  cout << "Usage: " << argv[0] <<
  " -d <device>\n"
  "\n"
  "Dump RDM traffic from a Enttec USB Pro device.\n"
  "\n"
  "  -a, --dump-all     Dump all packets (default is just RDM)\n"
  "  -d <device_path>   The path to the device.\n"
  "  -h, --help         Display this help message and exit.\n"
  "  -l, --log-level <level>  Set the loggging level 0 .. 4.\n"
  "  -v, --verbose      Show more packet details\n"
  << endl;
  exit(0);
}


void Stop(ola::network::SelectServer *ss) {
  ss->Terminate();
}


/*
 * Dump RDM data
 */
int main(int argc, char *argv[]) {
  options opts;
  opts.dump_all = false;
  opts.verbose = false;
  opts.log_level = ola::OLA_LOG_INFO;
  opts.help = false;
  opts.device = DEFAULT_DEVICE;
  ParseOptions(argc, argv, &opts);

  if (opts.help)
    DisplayHelpAndExit(argv);
  ola::InitLogging(opts.log_level, ola::OLA_LOG_STDERR);

  ola::network::ConnectedSocket *socket = ConnectToWidget(opts.device);
  if (!socket);
    exit(EX_UNAVAILABLE);

  ola::network::SelectServer ss;
  ss.AddSocket(socket);
  UsbWidget widget(socket);
  RDMSniffer sniffer(&widget, &ss, opts.dump_all, opts.verbose);

  widget.SetOnRemove(ola::NewSingleCallback(&Stop, &ss));
  ss.Run();

  return EX_OK;
}
