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
 * artnetdevice.cpp
 * Art-Net device
 * Copyright (C) 2005  Simon Newton
 *
 * An Art-Net device is an instance of libartnet bound to a single IP address
 * Art-Net is limited to four ports per direction per IP, so in this case
 * our device has 8 ports :
 *
 * Ids 0-3 : Input ports (recv dmx)
 * Ids 4-7 : Output ports (send dmx)
 */

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <string>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
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

using google::protobuf::RpcController;
using google::protobuf::Closure;
using ola::network::AddressToString;
using ola::plugin::artnet::Request;
using ola::plugin::artnet::Reply;

const char ArtNetDevice::K_ALWAYS_BROADCAST_KEY[] = "always_broadcast";
const char ArtNetDevice::K_DEVICE_NAME[] = "ArtNet";
const char ArtNetDevice::K_IP_KEY[] = "ip";
const char ArtNetDevice::K_LONG_NAME_KEY[] = "long_name";
const char ArtNetDevice::K_SHORT_NAME_KEY[] = "short_name";
const char ArtNetDevice::K_SUBNET_KEY[] = "subnet";

/*
 * Create a new Artnet Device
 */
ArtNetDevice::ArtNetDevice(AbstractPlugin *owner,
                           ola::Preferences *preferences,
                           PluginAdaptor *plugin_adaptor):
  Device(owner, K_DEVICE_NAME),
  m_preferences(preferences),
  m_node(NULL),
  m_plugin_adaptor(plugin_adaptor),
  m_timeout_id(ola::network::INVALID_TIMEOUT) {
}


/*
 * Start this device
 * @return true on success, false on failure
 */
bool ArtNetDevice::StartHook() {
  string value = m_preferences->GetValue(K_SUBNET_KEY);
  unsigned int subnet;
  if (!ola::StringToInt(m_preferences->GetValue(K_SUBNET_KEY), &subnet))
      subnet = 0;

  ola::network::Interface interface;
  ola::network::InterfacePicker *picker =
    ola::network::InterfacePicker::NewPicker();
  if (!picker->ChooseInterface(&interface, m_preferences->GetValue(K_IP_KEY))) {
    delete picker;
    OLA_INFO << "Failed to find an interface";
    return false;
  }
  delete picker;

  m_node = new ArtNetNode(
      interface,
      m_plugin_adaptor,
      m_preferences->GetValueAsBool(K_ALWAYS_BROADCAST_KEY));
  m_node->SetSubnetAddress(subnet);
  m_node->SetShortName(m_preferences->GetValue(K_SHORT_NAME_KEY));
  m_node->SetLongName(m_preferences->GetValue(K_LONG_NAME_KEY));

  for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++) {
    AddPort(new ArtNetOutputPort(this, i, m_node));
    AddPort(new ArtNetInputPort(this,
                                i,
                                m_plugin_adaptor,
                                m_node));
  }

  if (!m_node->Start()) {
    DeleteAllPorts();
    delete m_node;
    m_node = NULL;
    return false;
  }

  stringstream str;
  str << K_DEVICE_NAME << " [" << interface.ip_address << "]";
  SetName(str.str());

  m_timeout_id = m_plugin_adaptor->RegisterRepeatingTimeout(
      POLL_INTERVAL,
      NewCallback(m_node, &ArtNetNode::SendPoll));
  return true;
}


/*
 * Stop this device
 */
void ArtNetDevice::PostPortStop() {
  if (m_timeout_id != ola::network::INVALID_TIMEOUT) {
    m_plugin_adaptor->RemoveTimeout(m_timeout_id);
    m_timeout_id = ola::network::INVALID_TIMEOUT;
  }
  m_node->Stop();
  delete m_node;
  m_node = NULL;
}


/*
 * Handle device config messages
 * @param controller An RpcController
 * @param request the request data
 * @param response the response to return
 * @param done the closure to call once the request is complete
 */
void ArtNetDevice::Configure(RpcController *controller,
                             const string &request,
                             string *response,
                             Closure *done) {
    Request request_pb;
    if (!request_pb.ParseFromString(request)) {
      controller->SetFailed("Invalid Request");
      done->Run();
      return;
    }

    switch (request_pb.type()) {
      case ola::plugin::artnet::Request::ARTNET_OPTIONS_REQUEST:
        HandleOptions(&request_pb, response);
        break;
      default:
        controller->SetFailed("Invalid Request");
    }
    done->Run();
}


/*
 * Handle an options request
 */
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
  }

  ola::plugin::artnet::Reply reply;
  reply.set_type(ola::plugin::artnet::Reply::ARTNET_OPTIONS_REPLY);
  ola::plugin::artnet::OptionsReply *options_reply = reply.mutable_options();
  options_reply->set_status(status);
  options_reply->set_short_name(m_node->ShortName());
  options_reply->set_long_name(m_node->LongName());
  options_reply->set_subnet(m_node->SubnetAddress());
  reply.SerializeToString(response);
}
}  // artnet
}  // plugin
}  // ola
