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
 * ArtNetDevice.cpp
 * Art-Net device
 * Copyright (C) 2005 Simon Newton
 */

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "common/rpc/RpcController.h"
#include "ola/Callback.h"
#include "ola/CallbackRunner.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/NetworkUtils.h"
#include "olad/PluginAdaptor.h"
#include "olad/Port.h"
#include "olad/Preferences.h"
#include "plugins/artnet/ArtNetDevice.h"
#include "plugins/artnet/ArtNetPort.h"

namespace ola {
namespace plugin {
namespace artnet {

using ola::rpc::RpcController;
using ola::StringToInt;
using ola::network::IPV4Address;
using ola::plugin::artnet::Reply;
using ola::plugin::artnet::Request;
using std::auto_ptr;
using std::ostringstream;
using std::string;
using std::vector;

const char ArtNetDevice::K_ALWAYS_BROADCAST_KEY[] = "always_broadcast";
const char ArtNetDevice::K_DEVICE_NAME[] = "Art-Net";
const char ArtNetDevice::K_IP_KEY[] = "ip";
const char ArtNetDevice::K_LIMITED_BROADCAST_KEY[] = "use_limited_broadcast";
const char ArtNetDevice::K_LONG_NAME_KEY[] = "long_name";
const char ArtNetDevice::K_LOOPBACK_KEY[] = "use_loopback";
const char ArtNetDevice::K_NET_KEY[] = "net";
const char ArtNetDevice::K_OUTPUT_PORT_KEY[] = "output_ports";
const char ArtNetDevice::K_SHORT_NAME_KEY[] = "short_name";
const char ArtNetDevice::K_SUBNET_KEY[] = "subnet";
const unsigned int ArtNetDevice::K_ARTNET_NET = 0;
const unsigned int ArtNetDevice::K_ARTNET_SUBNET = 0;
const unsigned int ArtNetDevice::K_DEFAULT_OUTPUT_PORT_COUNT = 4;

ArtNetDevice::ArtNetDevice(AbstractPlugin *owner,
                           ola::Preferences *preferences,
                           PluginAdaptor *plugin_adaptor)
    : Device(owner, K_DEVICE_NAME),
      m_preferences(preferences),
      m_node(NULL),
      m_plugin_adaptor(plugin_adaptor),
      m_timeout_id(ola::thread::INVALID_TIMEOUT) {
}

bool ArtNetDevice::StartHook() {
  unsigned int subnet = StringToIntOrDefault(
      m_preferences->GetValue(K_SUBNET_KEY), K_ARTNET_SUBNET);

  unsigned int net = StringToIntOrDefault(
      m_preferences->GetValue(K_NET_KEY), K_ARTNET_NET);

  ola::network::Interface iface;
  auto_ptr<ola::network::InterfacePicker> picker(
      ola::network::InterfacePicker::NewPicker());
  ola::network::InterfacePicker::Options options;
  options.include_loopback = m_preferences->GetValueAsBool(K_LOOPBACK_KEY);
  if (!picker->ChooseInterface(&iface,
                               m_preferences->GetValue(K_IP_KEY),
                               options)) {
    OLA_INFO << "Failed to find an interface";
    return false;
  }

  ArtNetNodeOptions node_options;
  node_options.always_broadcast = m_preferences->GetValueAsBool(
      K_ALWAYS_BROADCAST_KEY);
  node_options.use_limited_broadcast_address = m_preferences->GetValueAsBool(
      K_LIMITED_BROADCAST_KEY);
  // OLA Output ports are Art-Net input ports
  node_options.input_port_count = StringToIntOrDefault(
      m_preferences->GetValue(K_OUTPUT_PORT_KEY),
      K_DEFAULT_OUTPUT_PORT_COUNT);

  m_node = new ArtNetNode(iface, m_plugin_adaptor, node_options);
  m_node->SetNetAddress(net);
  m_node->SetSubnetAddress(subnet);
  m_node->SetShortName(m_preferences->GetValue(K_SHORT_NAME_KEY));
  m_node->SetLongName(m_preferences->GetValue(K_LONG_NAME_KEY));

  for (unsigned int i = 0; i < node_options.input_port_count; i++) {
    AddPort(new ArtNetOutputPort(this, i, m_node));
  }

  for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++) {
    AddPort(new ArtNetInputPort(this, i, m_plugin_adaptor, m_node));
  }

  if (!m_node->Start()) {
    DeleteAllPorts();
    delete m_node;
    m_node = NULL;
    return false;
  }

  ostringstream str;
  str << K_DEVICE_NAME << " [" << iface.ip_address << "]";
  SetName(str.str());

  m_timeout_id = m_plugin_adaptor->RegisterRepeatingTimeout(
      POLL_INTERVAL,
      NewCallback(m_node, &ArtNetNode::SendPoll));
  return true;
}

void ArtNetDevice::PrePortStop() {
  if (m_timeout_id != ola::thread::INVALID_TIMEOUT) {
    m_plugin_adaptor->RemoveTimeout(m_timeout_id);
    m_timeout_id = ola::thread::INVALID_TIMEOUT;
  }
  m_node->Stop();
}

void ArtNetDevice::PostPortStop() {
  delete m_node;
  m_node = NULL;
}

void ArtNetDevice::Configure(RpcController *controller,
                             const string &request,
                             string *response,
                             ConfigureCallback *done) {
  CallbackRunner<ConfigureCallback> runner(done);
  Request request_pb;
  if (!request_pb.ParseFromString(request)) {
    controller->SetFailed("Invalid Request");
    return;
  }

  switch (request_pb.type()) {
    case ola::plugin::artnet::Request::ARTNET_OPTIONS_REQUEST:
      HandleOptions(&request_pb, response);
      break;
    case ola::plugin::artnet::Request::ARTNET_NODE_LIST_REQUEST:
      HandleNodeList(&request_pb, response, controller);
      break;
    default:
      controller->SetFailed("Invalid Request");
  }
}

void ArtNetDevice::HandleOptions(Request *request, string *response) {
  bool status = true;
  if (request->has_options()) {
    const ola::plugin::artnet::OptionsRequest options = request->options();
    if (options.has_short_name()) {
      status &= m_node->SetShortName(options.short_name());
    }
    if (options.has_long_name()) {
      status &= m_node->SetLongName(options.long_name());
    }
    if (options.has_subnet()) {
      status &= m_node->SetSubnetAddress(options.subnet());
    }
    if (options.has_net()) {
      status &= m_node->SetNetAddress(options.net());
    }
  }

  ola::plugin::artnet::Reply reply;
  reply.set_type(ola::plugin::artnet::Reply::ARTNET_OPTIONS_REPLY);
  ola::plugin::artnet::OptionsReply *options_reply = reply.mutable_options();
  options_reply->set_status(status);
  options_reply->set_short_name(m_node->ShortName());
  options_reply->set_long_name(m_node->LongName());
  options_reply->set_subnet(m_node->SubnetAddress());
  options_reply->set_net(m_node->NetAddress());
  reply.SerializeToString(response);
}

void ArtNetDevice::HandleNodeList(Request *request,
                                  string *response,
                                  RpcController *controller) {
  if (!request->has_node_list()) {
    controller->SetFailed("Missing NodeListRequest");
    return;
  }

  unsigned int universe_id = request->node_list().universe();
  vector<IPV4Address> node_addresses;

  vector<OutputPort*> output_ports;
  OutputPorts(&output_ports);
  vector<OutputPort*>::const_iterator port_iter = output_ports.begin();
  for (; port_iter != output_ports.end(); port_iter++) {
    Universe *universe = (*port_iter)->GetUniverse();
    if (universe && universe->UniverseId() == universe_id) {
      m_node->GetSubscribedNodes((*port_iter)->PortId(), &node_addresses);
      break;
    }
  }

  ola::plugin::artnet::Reply reply;
  reply.set_type(ola::plugin::artnet::Reply::ARTNET_NODE_LIST_REPLY);
  ola::plugin::artnet::NodeListReply *node_list_reply =
      reply.mutable_node_list();
  vector<IPV4Address>::const_iterator iter = node_addresses.begin();
  for (; iter != node_addresses.end(); ++iter) {
    OutputNode *node = node_list_reply->add_node();
    node->set_ip_address(iter->AsInt());
  }
  reply.SerializeToString(response);
}
}  // namespace artnet
}  // namespace plugin
}  // namespace ola
