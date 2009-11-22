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
#include <string>

#include "ola/Closure.h"
#include "ola/Logging.h"
#include "olad/Preferences.h"
#include "artnet/artnet.h"
#include "plugins/artnet/ArtNetDevice.h"
#include "plugins/artnet/ArtNetPort.h"


/*
 * Handle dmx from the network, called from libartnet
 */
int dmx_handler(artnet_node n, int prt, void *d) {
  ola::plugin::artnet::ArtNetDevice *device =
    reinterpret_cast<ola::plugin::artnet::ArtNetDevice*>(d);

  // don't return non zero here else libartnet will stop processing
  // this should never happen anyway
  if (prt < 0 || prt > ARTNET_MAX_PORTS)
    return 0;

  // signal to the port that the data has changed
  ola::plugin::artnet::ArtNetPort *port =
    reinterpret_cast<ola::plugin::artnet::ArtNetPort*>(device->GetPort(prt));
  port->DmxChanged();
  return 0;
}


/*
 * Notify of remote programming
 */
int program_handler(artnet_node n, void *d) {
  return 0;
}


namespace ola {
namespace plugin {
namespace artnet {

using google::protobuf::RpcController;
using google::protobuf::Closure;
using ola::plugin::artnet::Request;
using ola::plugin::artnet::Reply;

const char ArtNetDevice::K_SHORT_NAME_KEY[] = "short_name";
const char ArtNetDevice::K_LONG_NAME_KEY[] = "long_name";
const char ArtNetDevice::K_SUBNET_KEY[] = "subnet";
const char ArtNetDevice::K_IP_KEY[] = "ip";

/*
 * Create a new Artnet Device
 */
ArtNetDevice::ArtNetDevice(AbstractPlugin *owner,
                           const string &name,
                           ola::Preferences *preferences,
                           bool debug):
  Device(owner, name),
  m_preferences(preferences),
  m_socket(NULL),
  m_node(NULL),
  m_short_name(""),
  m_long_name(""),
  m_subnet(0),
  m_enabled(false),
  m_debug(debug) {
}


/*
 * Cleanup
 */
ArtNetDevice::~ArtNetDevice() {
  if (m_enabled)
    Stop();
}


/*
 * Start this device
 * @return true on success, false on failure
 */
bool ArtNetDevice::Start() {
  ArtNetPort *port;
  string value;
  int subnet = 0;

  /* set up ports */
  for (unsigned int i = 0; i < 2 * ARTNET_MAX_PORTS; i++) {
    port = new ArtNetPort(this, i);
    this->AddPort(port);
  }

  // create new artnet node, and and set config values
  if (m_preferences->GetValue(K_IP_KEY).empty()) {
    m_node = artnet_new(NULL, m_debug);
  } else {
    m_node = artnet_new(m_preferences->GetValue(K_IP_KEY).data(), m_debug);
  }

  if (!m_node) {
    OLA_WARN << "artnet_new failed " << artnet_strerror();
    goto e_dev;
  }

  // node config
  if (artnet_setoem(m_node, 0x04, 0x31)) {
    OLA_WARN << "artnet_setoem failed: " << artnet_strerror();
    goto e_artnet_start;
  }

  value = m_preferences->GetValue(K_SHORT_NAME_KEY);
  if (artnet_set_short_name(m_node, value.data())) {
    OLA_WARN << "artnet_set_short_name failed: " << artnet_strerror();
    goto e_artnet_start;
  }
  m_short_name = value;

  value = m_preferences->GetValue(K_LONG_NAME_KEY);
  if (artnet_set_long_name(m_node, value.data())) {
    OLA_WARN << "artnet_set_long_name failed: " << artnet_strerror();
    goto e_artnet_start;
  }
  m_long_name = value;

  if (artnet_set_node_type(m_node, ARTNET_SRV)) {
    OLA_WARN << "artnet_set_node_type failed: " << artnet_strerror();
    goto e_artnet_start;
  }

  value = m_preferences->GetValue(K_SUBNET_KEY);
  subnet = atoi(value.data());
  if (artnet_set_subnet_addr(m_node, subnet)) {
    OLA_WARN << "artnet_set_subnet_addr failed: " << artnet_strerror();
    goto e_artnet_start;
  }
  m_subnet = subnet;

  // we want to be notified when the node config changes
  if (artnet_set_program_handler(m_node, ::program_handler,
                                 reinterpret_cast<void*>(this))) {
    OLA_WARN << "artnet_set_program_handler failed: " << artnet_strerror();
    goto e_artnet_start;
  }

  if (artnet_set_dmx_handler(m_node, ::dmx_handler,
                             reinterpret_cast<void*>(this))) {
    OLA_WARN << "artnet_set_dmx_handler failed: " << artnet_strerror();
    goto e_artnet_start;
  }

  for (int i = 0; i < ARTNET_MAX_PORTS; i++) {
    // output ports
    if (artnet_set_port_type(m_node, i, ARTNET_ENABLE_OUTPUT,
                             ARTNET_PORT_DMX)) {
      OLA_WARN << "artnet_set_port_type failed %s", artnet_strerror();
      goto e_artnet_start;
    }

    if (artnet_set_port_addr(m_node, i, ARTNET_OUTPUT_PORT, i)) {
      OLA_WARN << "artnet_set_port_addr failed %s", artnet_strerror();
      goto e_artnet_start;
    }

    if (artnet_set_port_type(m_node, i, ARTNET_ENABLE_INPUT,
                             ARTNET_PORT_DMX)) {
      OLA_WARN << "artnet_set_port_type failed %s", artnet_strerror();
      goto e_artnet_start;
    }

    if (artnet_set_port_addr(m_node, i, ARTNET_INPUT_PORT, i)) {
      OLA_WARN << "artnet_set_port_addr failed %s", artnet_strerror();
      goto e_artnet_start;
    }
  }

  if (artnet_start(m_node)) {
    OLA_WARN << "artnet_start failed: " << artnet_strerror();
    goto e_artnet_start;
  }

  m_socket = new ola::network::UnmanagedSocket(artnet_get_sd(m_node));
  m_socket->SetOnData(NewClosure(this, &ArtNetDevice::SocketReady));
  m_enabled = true;
  return true;

 e_artnet_start:
  if (artnet_destroy(m_node))
    OLA_WARN << "artnet_destroy failed: " << artnet_strerror();

 e_dev:
  DeleteAllPorts();
  return false;
}


/*
 * Stop this device
 */
bool ArtNetDevice::Stop() {
  if (!m_enabled)
    return true;

  DeleteAllPorts();

  if (artnet_stop(m_node)) {
    OLA_WARN << "artnet_stop failed: " << artnet_strerror();
    return false;
  }

  if (artnet_destroy(m_node)) {
    OLA_WARN << "artnet_destroy failed: " << artnet_strerror();
    return false;
  }
  delete m_socket;
  m_socket = NULL;
  m_node = NULL;
  m_enabled = false;
  return true;
}


/*
 * Return the Art-Net node associated with this device
 */
artnet_node ArtNetDevice::GetArtnetNode() const {
  return m_node;
}


/*
 * Called when there is activity on our socket
 * @param socket the socket with activity
 */
int ArtNetDevice::SocketReady() {
  if (artnet_read(m_node, 0)) {
    OLA_WARN << "artnet_read failed: " << artnet_strerror();
    return -1;
  }
  return 0;
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
      if (artnet_set_short_name(m_node, options.short_name().data())) {
        OLA_WARN << "set short name failed: " << artnet_strerror();
        status = false;
      }
      m_short_name = options.short_name().substr(0,
                                                 ARTNET_SHORT_NAME_LENGTH - 1);
    }
    if (options.has_long_name()) {
      if (artnet_set_long_name(m_node, options.long_name().data())) {
        OLA_WARN << "set long name failed: " << artnet_strerror();
        status = false;
      }
      m_long_name = options.long_name().substr(0, ARTNET_LONG_NAME_LENGTH - 1);
    }
    if (options.has_subnet()) {
      if (artnet_set_subnet_addr(m_node, options.subnet())) {
        OLA_WARN << "set subnet failed: " << artnet_strerror();
        status = false;
      }
      m_subnet = options.subnet();
    }
  }

  ola::plugin::artnet::Reply reply;
  reply.set_type(ola::plugin::artnet::Reply::ARTNET_OPTIONS_REPLY);
  ola::plugin::artnet::OptionsReply *options_reply = reply.mutable_options();
  options_reply->set_status(status);
  options_reply->set_short_name(m_short_name);
  options_reply->set_long_name(m_long_name);
  options_reply->set_subnet(m_subnet);
  reply.SerializeToString(response);
}
}  // artnet
}  // plugin
}  // ola
