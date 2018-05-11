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
 * ola-artnet.cpp
 * Configure an Art-Net device
 * Copyright (C) 2005 Simon Newton
 */

#include <errno.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/network/IPV4Address.h>
#include <ola/plugin_id.h>
#include <plugins/artnet/messages/ArtNetConfigMessages.pb.h>
#include <iostream>
#include <string>
#include "examples/OlaConfigurator.h"

using std::cerr;
using std::cout;
using std::endl;
using std::string;

DECLARE_int32(device);
DEFINE_s_string(name, n, "", "Set the name of the Art-Net device.");
DEFINE_string(long_name, "", "Set the long name of the Art-Net device.");
DEFINE_int32(net, -1, "Set the net parameter of the Art-Net device.");
DEFINE_s_int32(subnet, s, -1,
               "Set the subnet parameter of the Art-Net device.");
DEFINE_s_uint32(universe, u, 0,
                "List the IPs of Art-Net devices for this universe.");

/*
 * A class that configures Art-Net devices
 */
class ArtnetConfigurator: public OlaConfigurator {
 public:
  ArtnetConfigurator()
      : OlaConfigurator(FLAGS_device, ola::OLA_PLUGIN_ARTNET) {}
  void HandleConfigResponse(const string &reply, const string &error);
  void SendConfigRequest();
 private:
  void SendOptionRequest();
  void SendNodeListRequest();
  void DisplayOptions(const ola::plugin::artnet::OptionsReply &reply);
  void DisplayNodeList(const ola::plugin::artnet::NodeListReply &reply);
};


/*
 * Handle the device config reply
 */
void ArtnetConfigurator::HandleConfigResponse(const string &reply,
                                              const string &error) {
  Terminate();
  if (!error.empty()) {
    cerr << error << endl;
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
  } else if (reply_pb.type() ==
                 ola::plugin::artnet::Reply::ARTNET_NODE_LIST_REPLY &&
             reply_pb.has_node_list()) {
    DisplayNodeList(reply_pb.node_list());
  } else {
    cout << "Invalid response type or missing options field" << endl;
  }
}


/*
 * Send a request
 */
void ArtnetConfigurator::SendConfigRequest() {
  if (FLAGS_universe.present()) {
    SendNodeListRequest();
  } else {
    SendOptionRequest();
  }
}

/**
 * Send an options request, which may involve setting options
 */
void ArtnetConfigurator::SendOptionRequest() {
  ola::plugin::artnet::Request request;
  request.set_type(ola::plugin::artnet::Request::ARTNET_OPTIONS_REQUEST);
  ola::plugin::artnet::OptionsRequest *options = request.mutable_options();

  if (FLAGS_name.present())
    options->set_short_name(FLAGS_name.str());
  if (FLAGS_long_name.present())
    options->set_long_name(FLAGS_long_name.str());
  if (FLAGS_subnet.present())
    options->set_subnet(FLAGS_subnet);
  if (FLAGS_net.present())
    options->set_net(FLAGS_net);
  SendMessage(request);
}


/**
 * Send a request for the node list
 */
void ArtnetConfigurator::SendNodeListRequest() {
  ola::plugin::artnet::Request request;
  request.set_type(ola::plugin::artnet::Request::ARTNET_NODE_LIST_REQUEST);
  ola::plugin::artnet::NodeListRequest *node_list_request =
      request.mutable_node_list();
  node_list_request->set_universe(FLAGS_universe);
  SendMessage(request);
}


/**
 * Display the widget parameters
 */
void ArtnetConfigurator::DisplayOptions(
    const ola::plugin::artnet::OptionsReply &reply) {
  cout << "Name: " << reply.short_name() << endl;
  cout << "Long Name: " << reply.long_name() << endl;
  cout << "Subnet: " << reply.subnet() << endl;
  cout << "Net: " << reply.net() << endl;
}


/**
 * Display the list of discovered nodes
 */
void ArtnetConfigurator::DisplayNodeList(
    const ola::plugin::artnet::NodeListReply &reply) {
  unsigned int nodes = reply.node_size();
  for (unsigned int i = 0; i < nodes; i++) {
    const ola::plugin::artnet::OutputNode &node = reply.node(i);
    ola::network::IPV4Address address(node.ip_address());
    cout << address << endl;
  }
}


/*
 * The main function
 */
int main(int argc, char*argv[]) {
  ola::AppInit(&argc,
               argv,
               "-d <dev_id> -n <name> -l <long_name> -s <subnet>",
               "Configure Art-Net devices managed by OLA.");

  if (FLAGS_device < 0)
    ola::DisplayUsageAndExit();

  ArtnetConfigurator configurator;
  if (!configurator.Setup()) {
    cerr << "Error: " << strerror(errno) << endl;
    exit(1);
  }

  configurator.Run();
  return 0;
}
