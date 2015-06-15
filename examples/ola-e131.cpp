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
 * ola-e131.cpp
 * Configure an E1.31 device
 * Copyright (C) 2005 Simon Newton
 */

#include <errno.h>
#include <ola/plugin_id.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <plugins/e131/messages/E131ConfigMessages.pb.h>
#include <iostream>
#include <string>
#include "examples/OlaConfigurator.h"

using std::cerr;
using std::cout;
using std::endl;
using std::string;

DECLARE_int32(device);
DEFINE_s_uint32(port_id, p, 0, "Id of the port to control");
DEFINE_s_default_bool(input, i, false,
                      "Set an input port, otherwise set an output port.");
DEFINE_bool(preview_mode, false, "Set the preview mode bit on|off");
DEFINE_default_bool(discovery, false, "Get the discovery state");

/*
 * A class that configures E131 devices
 */
class E131Configurator: public OlaConfigurator {
 public:
  E131Configurator()
      : OlaConfigurator(FLAGS_device, ola::OLA_PLUGIN_E131) {}
  void HandleConfigResponse(const string &reply, const string &error);
  void SendConfigRequest();
 private:
  void DisplayOptions(const ola::plugin::e131::PortInfoReply &reply);
  void DisplaySourceList(const ola::plugin::e131::SourceListReply &reply);
};


/*
 * Handle the device config reply
 */
void E131Configurator::HandleConfigResponse(const string &reply,
                                            const string &error) {
  Terminate();
  if (!error.empty()) {
    cerr << error << endl;
    return;
  }
  ola::plugin::e131::Reply reply_pb;
  if (!reply_pb.ParseFromString(reply)) {
    cout << "Protobuf parsing failed" << endl;
    return;
  }

  switch (reply_pb.type()) {
    case ola::plugin::e131::Reply::E131_PORT_INFO:
      if (reply_pb.has_port_info()) {
        DisplayOptions(reply_pb.port_info());
      } else {
        cout << "Missing port_info field in reply" << endl;
      }
      break;
    case ola::plugin::e131::Reply::E131_SOURCES_LIST:
      if (reply_pb.has_source_list()) {
        DisplaySourceList(reply_pb.source_list());
      } else {
        cout << "Missing source_list field in reply" << endl;
      }
      break;
    default:
      cout << "Invalid response type" << endl;
  }
}


/*
 * Send a get parameters request
 * @param device_id the device to send the request to
 * @param client the OLAClient
 */
void E131Configurator::SendConfigRequest() {
  ola::plugin::e131::Request request;

  if (FLAGS_preview_mode.present()) {
    if (FLAGS_port_id.present()) {
      request.set_type(ola::plugin::e131::Request::E131_PREVIEW_MODE);
      ola::plugin::e131::PreviewModeRequest *preview_request =
          request.mutable_preview_mode();
      preview_request->set_port_id(FLAGS_port_id);
      preview_request->set_preview_mode(FLAGS_preview_mode);
      preview_request->set_input_port(FLAGS_input);
    } else {
      cout << "Please specify a port number" << endl;
      request.set_type(ola::plugin::e131::Request::E131_PORT_INFO);
    }
  } else if (FLAGS_discovery) {
    request.set_type(ola::plugin::e131::Request::E131_SOURCES_LIST);
    ola::plugin::e131::SourceListRequest *source_list_request =
        request.mutable_source_list();
    (void) source_list_request;  // no options for now.
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

void E131Configurator::DisplaySourceList(
    const ola::plugin::e131::SourceListReply &reply) {
  if (reply.unsupported()) {
    cout << "Discovery mode isn't enabled" << endl;
    return;
  }

  for (int i = 0; i < reply.source_size(); i++) {
    const ola::plugin::e131::SourceEntry &entry = reply.source(i);
    cout << entry.cid() << " (" << entry.ip_address() << ")";
    if (entry.has_source_name()) {
      cout << ", " << entry.source_name();
    }
    cout << endl;
    for (int j = 0; j < entry.universe_size(); j++) {
      cout << "  " << entry.universe(j) << endl;
    }
  }
}

/*
 * The main function
 */
int main(int argc, char*argv[]) {
  ola::AppInit(
      &argc,
      argv,
      "-d <dev-id> -p <port-id> [--input] --preview-mode <on|off>",
      "Configure E1.31 devices managed by OLA.");

  if (FLAGS_device < 0)
    ola::DisplayUsageAndExit();

  E131Configurator configurator;
  if (!configurator.Setup()) {
    cerr << "Error: " << strerror(errno) << endl;
    exit(1);
  }

  configurator.Run();
  return 0;
}
