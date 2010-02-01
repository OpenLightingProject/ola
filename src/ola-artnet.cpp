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
 *  ola-artnet.cpp
 *  Configure an ArtNet device
 *  Copyright (C) 2005-2009 Simon Newton
 */

#include <errno.h>
#include <getopt.h>
#include <ola/artnet/ArtnetConfigMessages.pb.h>
#include <ola/plugin_id.h>
#include <iostream>
#include <string>
#include "src/OlaConfigurator.h"

using std::string;

typedef struct {
  string command;   // argv[0]
  int device_id;    // device id
  bool help;        // help
  bool has_name;
  string name;      // short name
  bool has_long_name;
  string long_name;  // long name
  bool has_subnet;
  int subnet;       // the subnet
} options;


/*
 * A class that configures Artnet devices
 */
class ArtnetConfigurator: public OlaConfigurator {
  public:
    explicit ArtnetConfigurator(const options &opts):
      OlaConfigurator(opts.device_id, ola::OLA_PLUGIN_ARTNET),
      m_options(opts) {}
    void HandleConfigResponse(const string &reply, const string &error);
    void SendConfigRequest();
  private:
    void DisplayOptions(const ola::plugin::artnet::OptionsReply &reply);
    options m_options;
};


/*
 * Handle the device config reply
 */
void ArtnetConfigurator::HandleConfigResponse(const string &reply,
                                              const string &error) {
  Terminate();
  if (!error.empty()) {
    cout << error << endl;
    return;
  }
  ola::plugin::artnet::Reply reply_pb;
  if (!reply_pb.ParseFromString(reply)) {
    cout << "Protobuf parsing failed" << endl;
    return;
  }
  if (reply_pb.type() == ola::plugin::artnet::Reply::ARTNET_OPTIONS_REPLY &&
      reply_pb.has_options()) {
    DisplayOptions(reply_pb.options());
    return;
  }
  cout << "Invalid response type or missing options field" << endl;
}


/*
 * Send a get parameters request
 * @param device_id the device to send the request to
 * @param client the OLAClient
 */
void ArtnetConfigurator::SendConfigRequest() {
  ola::plugin::artnet::Request request;
  request.set_type(ola::plugin::artnet::Request::ARTNET_OPTIONS_REQUEST);
  ola::plugin::artnet::OptionsRequest *options = request.mutable_options();

  if (m_options.has_name)
    options->set_short_name(m_options.name);
  if (m_options.has_long_name)
    options->set_long_name(m_options.long_name);
  if (m_options.has_subnet)
    options->set_subnet(m_options.subnet);
  SendMessage(request);
}


/*
 * Display the widget parameters
 */
void ArtnetConfigurator::DisplayOptions(
    const ola::plugin::artnet::OptionsReply &reply) {
  cout << "Name: " << reply.short_name() << endl;
  cout << "Long Name: " << reply.long_name() << endl;
  cout << "Subnet: " << reply.subnet() << endl;
}


/*
 * Parse our cmd line options
 */
int ParseOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"dev",       required_argument,  0, 'd'},
      {"help",      no_argument,        0, 'h'},
      {"long_name", required_argument,  0, 'l'},
      {"name",      required_argument,  0, 'n'},
      {"subnet",    required_argument,  0, 's'},
      {0, 0, 0, 0}
    };

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "d:hl:n:s:v", long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'd':
        opts->device_id = atoi(optarg);
        break;
      case 'h':
        opts->help = true;
        break;
      case 'l':
        opts->long_name = optarg;
        opts->has_long_name = true;
        break;
      case 'n':
        opts->name = optarg;
        opts->has_name = true;
        break;
      case 's':
        opts->subnet = atoi(optarg);
        opts->has_subnet = true;
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
void DisplayHelpAndExit(const options &opts) {
  cout << "Usage: " << opts.command <<
    "-d <dev_id> -n <name> -l <long_name> -s <subnet>\n\n"
    "Configure ArtNet Devices managed by OLA.\n\n"
    "  -h, --help      Display this help message and exit.\n"
    "  -l, --long_name Set the long name of the ArtNet device\n"
    "  -n, --name      Set the name of the ArtNet device\n"
    "  -s, --subnet    Set the subnet of the ArtNet device\n" <<
    endl;
  exit(0);
}


/*
 * The main function
 */
int main(int argc, char*argv[]) {
  options opts;
  opts.command = argv[0];
  opts.device_id = -1;
  opts.help = false;
  opts.has_name = false;
  opts.has_long_name = false;
  opts.has_subnet = false;

  ParseOptions(argc, argv, &opts);

  if (opts.help || opts.device_id < 0)
    DisplayHelpAndExit(opts);

  ArtnetConfigurator configurator(opts);
  if (!configurator.Setup()) {
    cout << "error: " << strerror(errno) << endl;
    exit(1);
  }

  configurator.Run();
  return 0;
}
