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
 * SandNetNode.cpp
 * A SandNet node
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string.h>
#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include <ola/Logging.h>
#include <ola/network/NetworkUtils.h>
#include "SandNetNode.h"


namespace ola {
namespace plugin {
namespace sandnet {

using std::string;
using std::map;
using std::vector;
using ola::network::UdpSocket;
using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using ola::Closure;

const char SandNetNode::CONTROL_ADDRESS[] = "237.1.1.1";
const char SandNetNode::DATA_ADDRESS[] = "237.1.2.1";
const char SandNetNode::DEFAULT_NODE_NAME[] = "ola-SandNet";

/*
 * Create a new node
 * @param ip_address the IP address to prefer to listen on, if NULL we choose
 * one.
 */
SandNetNode::SandNetNode(const string &ip_address)
    : m_running(false),
      m_node_name(DEFAULT_NODE_NAME),
      m_preferred_ip(ip_address) {
  for(unsigned int i=0; i < SANDNET_MAX_PORTS; i++) {
    m_ports[i].group = 0;
    m_ports[i].universe = i;
  }
}


/*
 * Cleanup
 */
SandNetNode::~SandNetNode() {
  Stop();

  universe_handlers::iterator iter;
  for (iter = m_handlers.begin(); iter != m_handlers.end(); ++iter) {
    delete iter->second.closure;
  }
  m_handlers.clear();
}


/*
 * Start this node
 */
bool SandNetNode::Start() {
  if (m_running)
    return false;

  if (!m_interface_picker.ChooseInterface(&m_interface, m_preferred_ip)) {
    OLA_INFO << "Failed to find an interface";
    return false;
  }

  if (!InitNetwork())
    return false;

  m_running = true;
  return true;
}


/*
 * Stop this node
 */
bool SandNetNode::Stop() {
  if (!m_running)
    return false;

  m_data_socket.Close();
  m_control_socket.Close();

  m_running = false;
  return true;
}


/*
 * Return a list of sockets in use
 */
vector<UdpSocket*> SandNetNode::GetSockets() {
  vector<UdpSocket*> sockets;
  sockets.push_back(&m_data_socket);
  sockets.push_back(&m_control_socket);
  return sockets;
}


/*
 * Called when there is data on this socket
 */
int SandNetNode::SocketReady(UdpSocket *socket) {
  sandnet_packet packet;
  ssize_t packet_size = sizeof(packet);
  struct sockaddr_in source;
  socklen_t source_length = sizeof(source);

  if(!socket->RecvFrom((uint8_t*) &packet, &packet_size, source,
                       source_length))
    return -1;

  // skip packets sent by us
  if (source.sin_addr.s_addr == m_interface.ip_address.s_addr)
    return 0;

  if (packet_size < static_cast<ssize_t>(sizeof(packet.opcode))) {
    OLA_WARN << "Small sandnet packet received, discarding";
    return -1;
  }

  switch (NetworkToHost(packet.opcode)) {
    case SANDNET_DMX:
      HandleDMX(packet.contents.dmx, packet_size - sizeof(packet.opcode));
      break;
    case SANDNET_COMPRESSED_DMX:
      HandleCompressedDMX(packet.contents.compressed_dmx,
                          packet_size - sizeof(packet.opcode));
      break;
    default:
      OLA_INFO << "Skipping sandnet packet with unknown code: 0x" <<
        std::hex << NetworkToHost(packet.opcode);
  }
  return 0;
}


/*
 * Set the closure to be called when we receive data for this universe.
 * @param universe the universe to register the handler for
 * @param handler the Closure to call when there is data for this universe.
 * Ownership of the closure is transferred to the node.
 */
bool SandNetNode::SetHandler(uint8_t group, uint8_t universe,
                             Closure *closure) {
  if (!closure)
    return false;

  group_universe_pair key(group, universe);
  universe_handlers::iterator iter = m_handlers.find(key);

  if (iter == m_handlers.end()) {
    universe_handler handler;
    handler.closure = closure;
    handler.buffer.Blackout();
    m_handlers[key] = handler;
  } else {
    Closure *old_closure = iter->second.closure;
    iter->second.closure = closure;
    delete old_closure;
  }
  return true;
}


/*
 * Remove the handler for this universe
 * @param universe the universe handler to remove
 * @param true if removed, false if it didn't exist
 */
bool SandNetNode::RemoveHandler(uint8_t group, uint8_t universe) {
  group_universe_pair key(group, universe);
  universe_handlers::iterator iter = m_handlers.find(key);

  if (iter != m_handlers.end()) {
    Closure *old_closure = iter->second.closure;
    m_handlers.erase(iter);
    delete old_closure;
    return true;
  }
  return false;
}


/*
 * Get the DMX data for this universe.
 */
DmxBuffer SandNetNode::GetDMX(uint8_t group, uint8_t universe) const {
  group_universe_pair key(group, universe);
  universe_handlers::const_iterator iter = m_handlers.find(key);

  if (iter != m_handlers.end())
    return iter->second.buffer;
  else {
    DmxBuffer buffer;
    return buffer;
  }
}


/*
 * Set the parameters for a port
 */
bool SandNetNode::SetPortParameters(uint8_t port_id, sandnet_port_type type,
                                    uint8_t group, uint8_t universe) {

  if (port_id >= SANDNET_MAX_PORTS)
    return false;

  m_ports[port_id].group = group;
  m_ports[port_id].universe = universe;
  m_ports[port_id].type = type;
  return true;
}


/*
 * Send a Sandnet Advertisment.
 */
int SandNetNode::SendAdvertisement() {
  if (!m_running)
    return -1;

  sandnet_packet packet;
  memset(&packet, 0, sizeof(packet));
  packet.opcode = HostToNetwork(static_cast<uint16_t>(SANDNET_ADVERTISMENT));




  return 0;
}


/*
 * Send some DMX data
 * @param buffer the DMX data
 * @return true if it was send successfully, false otherwise
 */
bool SandNetNode::SendDMX(uint8_t port_id, const DmxBuffer &buffer) {
  if (!m_running || port_id >= SANDNET_MAX_PORTS)
    return false;

  // Sandnet doesn't seem to understand compressed DMX
  return SendUncompressedDMX(port_id, buffer);
}


/*
 * Setup the networking compoents.
 */
bool SandNetNode::InitNetwork() {
  if (!m_control_socket.Init()) {
    OLA_WARN << "Socket init failed";
    return false;
  }

  if (!m_data_socket.Init()) {
    OLA_WARN << "Socket init failed";
    m_control_socket.Close();
    return false;
  }

  if (!m_control_socket.Bind(CONTROL_PORT)) {
    OLA_WARN << "Failed to bind to:" << CONTROL_PORT;
    m_data_socket.Close();
    m_control_socket.Close();
    return false;
  }

  if (!m_data_socket.Bind(DATA_PORT)) {
    OLA_WARN << "Failed to bind to:" << DATA_PORT;
    m_data_socket.Close();
    m_control_socket.Close();
    return false;
  }

  //JoinMulticast()

  m_control_socket.SetOnData(
    NewClosure(this, &SandNetNode::SocketReady, &m_control_socket));
  m_data_socket.SetOnData(
    NewClosure(this, &SandNetNode::SocketReady, &m_data_socket));
  return true;
}


bool SandNetNode::HandleCompressedDMX(const sandnet_compressed_dmx &dmx_packet,
                                      unsigned int size) {


}


bool SandNetNode::HandleDMX(const sandnet_dmx &dmx_packet,
                            unsigned int size) {

}


bool SandNetNode::SendUncompressedDMX(uint8_t port_id,
                                      const DmxBuffer &buffer) {

  sandnet_packet packet;
  sandnet_dmx *dmx_packet = &packet.contents.dmx;

  packet.opcode = HostToNetwork(static_cast<uint16_t>(SANDNET_DMX));
  dmx_packet->group = m_ports[port_id].group;
  dmx_packet->universe = m_ports[port_id].universe;
  dmx_packet->port = port_id;

  unsigned int length = DMX_UNIVERSE_SIZE;
  buffer.Get(dmx_packet->dmx, &length);

  unsigned int header_size = sizeof(sandnet_dmx) - sizeof(sandnet_dmx.dmx);
  return SendPacket(packet, sizeof(packet.opcode) + header_size + length);
}



/*
 * Send a packet
 */
bool SandNetNode::SendPacket(const sandnet_packet &packet,
                             unsigned int size,
                             bool is_control) {
  struct sockaddr_in destination;
  destination.sin_family = AF_INET;

  UdpSocket *socket;

  if (is_control) {
    destination.sin_port = HostToNetwork(CONTROL_PORT);
    destination.sin_addr = m_control_addr;
    socket = &m_control_socket;
  } else {
    destination.sin_port = HostToNetwork(DATA_PORT);
    destination.sin_addr = m_data_addr;
    socket = m_data_socket;
  }

  ssize_t bytes_sent = socket->SendTo((uint8_t*) &packet,
                                        size,
                                        destination);

  if (bytes_sent != size) {
    OLA_WARN << "Only sent " << bytes_sent << " of " << size;
    return false;
  }
  return true;
}

}  // sandnet
}  // plugin
}  // ola
