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
 *  ola-e131.cpp
 *  Configure an E1.31 device
 *  Copyright (C) 2005-2009 Simon Newton
 */

#include <errno.h>
#include <getopt.h>
#include <ola/plugin_id.h>
#include <plugins/e131/messages/E131ConfigMessages.pb.h>
#include <iostream>
#include <string>
#include "examples/OlaConfigurator.h"

using std::cout;
using std::endl;
using std::string;

typedef enum {
  PORT_INFO,
  PORT_PREVIEW_MODE,
} config_mode;

typedef struct {
  string command;   // argv[0]
  int device_id;    // device id
  bool help;        // help
  config_mode mode;
  unsigned int port_id;  // port_id
  bool preview_mode;  // set preview mode
  bool input_port;  // use the input port rather than output one
} options;


/*
 * A class that configures E131 devices
 */
class E131Configurator: public OlaConfigurator {
  public:
    explicit E131Configurator(const options &opts):
      OlaConfigurator(opts.device_id, ola::OLA_PLUGIN_E131),
      m_options(opts) {}
    void HandleConfigResponse(const string &reply, const string &error);
    void SendConfigRequest();
  private:
    void DisplayOptions(const ola::plugin::e131::PortInfoReply &reply);
    options m_options;
};


/*
 * Handle the device config reply
 */
void E131Configurator::HandleConfigResponse(const string &reply,
                                            const string &error) {
  Terminate();
  if (!error.empty()) {
    cout << error << endl;
    return;
  }
  ola::plugin::e131::Reply reply_pb;
  if (!reply_pb.ParseFromString(reply)) {
    cout << "Protobuf parsing failed" << endl;
    return;
  }
  if (reply_pb.type() == ola::plugin::e131::Reply::E131_PORT_INFO &&
      reply_pb.has_port_info()) {
    DisplayOptions(reply_pb.port_info());
    return;
  }
  cout << "Invalid response type or missing port_info field" << endl;
}


/*
 * Send a get parameters request
 * @param device_id the device to send the request to
 * @param client the OLAClient
 */
void E131Configurator::SendConfigRequest() {
  ola::plugin::e131::Request request;

  if (m_options.mode == PORT_PREVIEW_MODE) {
    request.set_type(ola::plugin::e131::Request::E131_PREVIEW_MODE);
    ola::plugin::e131::PreviewModeRequest *preview_request =
      request.mutable_preview_mode();
    preview_request->set_port_id(m_options.port_id);
    preview_request->set_preview_mode(m_options.preview_mode);
    preview_request->set_input_port(m_options.input_port);
  } else {
    request.set_type(ola::plugin::e131::Request::E131_PORT_INFO);
  }
  SendMessage(request);
}


/*
 * Display the widget parameters
 */
void E131Configurator::DisplayOptions(
    const ola::plugin::e131::PortInfoReply &reply) {

  for (int i = 0; i < reply.input_port_size(); i++) {
    cout << "Input Port  " << reply.input_port(i).port_id() <<
      ", ignore preview mode " <<
      (reply.input_port(i).preview_mode() ? "on" : "off") << endl;
  }

  for (int i = 0; i < reply.output_port_size(); i++) {
    cout << "Output Port  " << reply.output_port(i).port_id() <<
      ", preview mode " << (reply.output_port(i).preview_mode() ? "on" : "off")
      << endl;
  }
}


/*
 * Parse our cmd line options
 */
int ParseOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"dev",       required_argument,  0, 'd'},
      {"help",      no_argument,        0, 'h'},
      {"input",     no_argument,        0, 'i'},
      {"port-id",   required_argument,  0, 'p'},
      {"preview_mode", required_argument,  0, 'm'},
      {0, 0, 0, 0}
    };

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "d:him:p:", long_options, &option_index);
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
      case 'i':
        opts->input_port = true;
        break;
      case 'p':
        opts->port_id = atoi(optarg);
        break;
      case 'm':
        opts->preview_mode = (string(optarg) == "on" ? true : false);
        opts->mode = PORT_PREVIEW_MODE;
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
    " -d <dev-id> -p <port-id> [--input] --preview_mode <on|off>\n\n"
    "Configure E1.31 Devices managed by OLA.\n\n"
    "  -d, --dev       Id of the device to control.\n"
    "  -h, --help      Display this help message and exit.\n"
    "  -i, --input     Input port\n"
    "  -p, --port-id   Id of the port to control\n"
    "  --preview_mode  Set the preview mode bit\n" <<
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
  opts.mode = PORT_INFO;
  opts.port_id = -1;
  opts.preview_mode = false;
  opts.input_port = false;

  ParseOptions(argc, argv, &opts);

  if (opts.help || opts.device_id < 0)
    DisplayHelpAndExit(opts);

  E131Configurator configurator(opts);
  if (!configurator.Setup()) {
    cout << "error: " << strerror(errno) << endl;
    exit(1);
  }

  configurator.Run();
  return 0;
}
