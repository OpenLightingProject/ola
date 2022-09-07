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
 * SandNetNode.cpp
 * A SandNet node
 * Copyright (C) 2005 Simon Newton
 */

#include <string.h>
#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include "ola/Logging.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/MACAddress.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/sandnet/SandNetNode.h"


namespace ola {
namespace plugin {
namespace sandnet {

using std::string;
using std::map;
using std::vector;
using ola::network::HostToNetwork;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::MACAddress;
using ola::network::NetworkToHost;
using ola::network::UDPSocket;
using ola::Callback0;

const uint16_t SandNetNode::CONTROL_PORT = 37895;
const uint16_t SandNetNode::DATA_PORT = 37900;
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
  for (unsigned int i = 0; i < SANDNET_MAX_PORTS; i++) {
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
  if (m_running) {
    return false;
  }

  ola::network::InterfacePicker *picker =
    ola::network::InterfacePicker::NewPicker();
  if (!picker->ChooseInterface(&m_interface, m_preferred_ip)) {
    delete picker;
    OLA_INFO << "Failed to find an interface";
    return false;
  }
  delete picker;

  IPV4Address ip;
  if (!IPV4Address::FromString(CONTROL_ADDRESS, &ip)) {
    OLA_WARN << "Could not convert " << CONTROL_ADDRESS;
    return false;
  }
  m_control_addr = IPV4SocketAddress(ip, CONTROL_PORT);

  if (!IPV4Address::FromString(DATA_ADDRESS, &ip)) {
    OLA_WARN << "Could not convert " << DATA_ADDRESS;
    return false;
  }
  m_data_addr = IPV4SocketAddress(ip, DATA_PORT);

  if (!InitNetwork()) {
    return false;
  }

  m_running = true;
  return true;
}


/*
 * Stop this node
 */
bool SandNetNode::Stop() {
  if (!m_running) {
    return false;
  }

  m_data_socket.Close();
  m_control_socket.Close();

  m_running = false;
  return true;
}


/*
 * Return a list of sockets in use
 */
vector<UDPSocket*> SandNetNode::GetSockets() {
  vector<UDPSocket*> sockets;
  sockets.push_back(&m_data_socket);
  sockets.push_back(&m_control_socket);
  return sockets;
}


/*
 * Called when there is data on this socket
 */
void SandNetNode::SocketReady(UDPSocket *socket) {
  sandnet_packet packet;
  ssize_t packet_size = sizeof(packet);
  IPV4SocketAddress source;

  if (!socket->RecvFrom(reinterpret_cast<uint8_t*>(&packet),
                        &packet_size, &source)) {
    return;
  }

  // skip packets sent by us
  if (source.Host() == m_interface.ip_address) {
    return;
  }

  if (packet_size < static_cast<ssize_t>(sizeof(packet.opcode))) {
    OLA_WARN << "Small sandnet packet received, discarding";
    return;
  }

  switch (NetworkToHost(packet.opcode)) {
    case SANDNET_DMX:
      HandleDMX(packet.contents.dmx, packet_size - sizeof(packet.opcode));
      break;
    case SANDNET_COMPRESSED_DMX:
      HandleCompressedDMX(packet.contents.compressed_dmx,
                          packet_size - sizeof(packet.opcode));
      break;
    case SANDNET_ADVERTISEMENT:
      break;
    default:
      OLA_INFO << "Skipping sandnet packet with unknown code: 0x" <<
        std::hex << NetworkToHost(packet.opcode);
  }
}


/*
 * Set the closure to be called when we receive data for this universe.
 * @param universe the universe to register the handler for
 * @param handler the Callback0 to call when there is data for this universe.
 * Ownership of the closure is transferred to the node.
 */
bool SandNetNode::SetHandler(uint8_t group, uint8_t universe,
                             DmxBuffer *buffer,
                             Callback0<void> *closure) {
  if (!closure) {
    return false;
  }

  group_universe_pair key(group, universe);
  universe_handlers::iterator iter = m_handlers.find(key);

  if (iter == m_handlers.end()) {
    universe_handler handler;
    handler.buffer = buffer;
    handler.closure = closure;
    m_handlers[key] = handler;
  } else {
    Callback0<void> *old_closure = iter->second.closure;
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
    Callback0<void> *old_closure = iter->second.closure;
    m_handlers.erase(iter);
    delete old_closure;
    return true;
  }
  return false;
}


/*
 * Set the parameters for a port
 */
bool SandNetNode::SetPortParameters(uint8_t port_id, sandnet_port_type type,
                                    uint8_t group, uint8_t universe) {
  if (port_id >= SANDNET_MAX_PORTS) {
    return false;
  }

  m_ports[port_id].group = group;
  m_ports[port_id].universe = universe;
  m_ports[port_id].type = type;
  return true;
}


/*
 * Send a Sandnet Advertisement.
 */
bool SandNetNode::SendAdvertisement() {
  if (!m_running) {
    return false;
  }

  sandnet_packet packet;
  sandnet_advertisement *advertisement = &packet.contents.advertisement;
  memset(&packet, 0, sizeof(packet));
  packet.opcode = HostToNetwork(static_cast<uint16_t>(SANDNET_ADVERTISEMENT));

  m_interface.hw_address.Get(advertisement->mac);
  advertisement->firmware = HostToNetwork(FIRMWARE_VERSION);

  for (unsigned int i = 0; i < SANDNET_MAX_PORTS; i++) {
    advertisement->ports[i].mode = m_ports[i].type;
    advertisement->ports[i].protocol = SANDNET_SANDNET;
    advertisement->ports[i].group = m_ports[i].group;
    advertisement->ports[i].universe = m_ports[i].universe;
  }

  advertisement->nlen = std::min(m_node_name.size(),
                                 static_cast<size_t>(SANDNET_NAME_LENGTH));
  strncpy(advertisement->name, m_node_name.data(), advertisement->nlen);

  advertisement->magic3[0] = 0xc0;
  advertisement->magic3[1] = 0xa8;
  advertisement->magic3[2] = 0x01;
  advertisement->magic3[3] = 0xa0;
  advertisement->magic3[4] = 0x00;
  advertisement->magic3[5] = 0xff;
  advertisement->magic3[6] = 0xff;
  advertisement->magic3[7] = 0xff;
  advertisement->magic3[8] = 0x00;
  advertisement->magic4 = 0x01;

  return SendPacket(packet,
                    sizeof(packet.opcode) + sizeof(sandnet_advertisement),
                    true);
}


/*
 * Send some DMX data
 * @param buffer the DMX data
 * @return true if it was sent successfully, false otherwise
 */
bool SandNetNode::SendDMX(uint8_t port_id, const DmxBuffer &buffer) {
  if (!m_running || port_id >= SANDNET_MAX_PORTS) {
    return false;
  }

  // Sandnet doesn't seem to understand compressed DMX
  return SendUncompressedDMX(port_id, buffer);
}


/*
 * Setup the networking components.
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

  if (!m_control_socket.Bind(IPV4SocketAddress(IPV4Address::WildCard(),
                                               CONTROL_PORT))) {
    m_data_socket.Close();
    m_control_socket.Close();
    return false;
  }

  if (!m_data_socket.Bind(IPV4SocketAddress(IPV4Address::WildCard(),
                                            DATA_PORT))) {
    m_data_socket.Close();
    m_control_socket.Close();
    return false;
  }

  if (!m_control_socket.SetMulticastInterface(m_interface.ip_address)) {
    m_data_socket.Close();
    m_control_socket.Close();
    return false;
  }

  if (!m_data_socket.SetMulticastInterface(m_interface.ip_address)) {
    m_data_socket.Close();
    m_control_socket.Close();
    return false;
  }

  if (!m_control_socket.JoinMulticast(m_interface.ip_address,
                                      m_control_addr.Host())) {
      OLA_WARN << "Failed to join multicast to: " << m_control_addr;
    m_data_socket.Close();
    m_control_socket.Close();
    return false;
  }

  if (!m_data_socket.JoinMulticast(m_interface.ip_address,
                                   m_data_addr.Host())) {
      OLA_WARN << "Failed to join multicast to: " << m_data_addr;
    m_data_socket.Close();
    m_control_socket.Close();
    return false;
  }

  m_control_socket.SetOnData(
    NewCallback(this, &SandNetNode::SocketReady, &m_control_socket));
  m_data_socket.SetOnData(
    NewCallback(this, &SandNetNode::SocketReady, &m_data_socket));
  return true;
}


/*
 * Handle a compressed DMX packet
 */
bool SandNetNode::HandleCompressedDMX(const sandnet_compressed_dmx &dmx_packet,
                                      unsigned int size) {
  unsigned int header_size = sizeof(dmx_packet) - sizeof(dmx_packet.dmx);

  if (size <= header_size) {
    OLA_WARN << "Sandnet data size too small, expected at least "
             << header_size << ", got " << size;
    return false;
  }

  group_universe_pair key(dmx_packet.group, dmx_packet.universe);
  universe_handlers::iterator iter = m_handlers.find(key);

  if (iter == m_handlers.end()) {
    return false;
  }

  unsigned int data_size = size - header_size;
  bool r = m_encoder.Decode(0, dmx_packet.dmx, data_size, iter->second.buffer);
  if (!r) {
    OLA_WARN << "Failed to decode Sandnet Data";
    return false;
  }

  iter->second.closure->Run();
  return true;
}


/*
 * Handle an uncompressed DMX packet
 */
bool SandNetNode::HandleDMX(const sandnet_dmx &dmx_packet,
                            unsigned int size) {
  unsigned int header_size = sizeof(dmx_packet) - sizeof(dmx_packet.dmx);
  if (size <= header_size) {
    OLA_WARN << "Sandnet data size too small, expected at least "
             << header_size << ", got " << size;
    return false;
  }

  group_universe_pair key(dmx_packet.group, dmx_packet.universe);
  universe_handlers::iterator iter = m_handlers.find(key);

  if (iter == m_handlers.end()) {
    return false;
  }

  unsigned int data_size = size - header_size;
  iter->second.buffer->Set(dmx_packet.dmx, data_size);
  iter->second.closure->Run();
  return true;
}


/*
 * Send an uncompressed DMX packet
 */
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

  unsigned int header_size = sizeof(sandnet_dmx) - sizeof(dmx_packet->dmx);
  return SendPacket(packet, sizeof(packet.opcode) + header_size + length);
}



/*
 * Send a packet
 */
bool SandNetNode::SendPacket(const sandnet_packet &packet,
                             unsigned int size,
                             bool is_control) {
  UDPSocket *socket;
  if (is_control) {
    socket = &m_control_socket;
  } else {
    socket = &m_data_socket;
  }

  ssize_t bytes_sent = socket->SendTo(
      reinterpret_cast<const uint8_t*>(&packet),
      size,
      is_control ? m_control_addr : m_data_addr);

  if (bytes_sent != static_cast<ssize_t>(size)) {
    OLA_INFO << "Only sent " << bytes_sent << " of " << size;
    return false;
  }
  return true;
}
}  // namespace sandnet
}  // namespace plugin
}  // namespace ola
