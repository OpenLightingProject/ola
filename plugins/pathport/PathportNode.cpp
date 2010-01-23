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
 * PathportNode.cpp
 * A SandNet node
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string.h>
#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include "ola/Logging.h"
#include "ola/BaseTypes.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/pathport/PathportNode.h"


namespace ola {
namespace plugin {
namespace pathport {

using std::string;
using std::map;
using std::vector;
using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using ola::network::StringToAddress;
using ola::network::UdpSocket;
using ola::Closure;

/*
 * Create a new node
 * @param ip_address the IP address to prefer to listen on, if NULL we choose
 * one.
 */
PathportNode::PathportNode(const string &ip_address,
                         uint32_t device_id)
    : m_running(false),
      m_preferred_ip(ip_address),
      m_device_id(device_id),
      m_sequence_number(1) {
}


/*
 * Cleanup
 */
PathportNode::~PathportNode() {
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
bool PathportNode::Start() {
  if (m_running)
    return false;

  if (!m_interface_picker.ChooseInterface(&m_interface, m_preferred_ip)) {
    OLA_INFO << "Failed to find an interface";
    return false;
  }

  m_config_addr.s_addr = HostToNetwork(PATHPORT_CONFIG_GROUP);
  m_status_addr.s_addr = HostToNetwork(PATHPORT_STATUS_GROUP);
  m_data_addr.s_addr = HostToNetwork(PATHPORT_DATA_GROUP);

  if (!InitNetwork())
    return false;

  m_running = true;
  SendArpReply();

  return true;
}


/*
 * Stop this node
 */
bool PathportNode::Stop() {
  if (!m_running)
    return false;

  m_socket.Close();
  m_running = false;
  return true;
}


/*
 * Called when there is data on this socket
 */
int PathportNode::SocketReady(UdpSocket *socket) {
  pathport_packet_s packet;
  ssize_t packet_size = sizeof(packet);
  struct sockaddr_in source;
  socklen_t source_length = sizeof(source);

  if (!socket->RecvFrom(reinterpret_cast<uint8_t*>(&packet),
                        &packet_size,
                        source,
                        source_length))
    return -1;

  // skip packets sent by us
  if (source.sin_addr.s_addr == m_interface.ip_address.s_addr)
    return 0;

  if (packet_size < static_cast<ssize_t>(sizeof(packet.header))) {
    OLA_WARN << "Small pathport packet received, discarding";
    return -1;
  }
  packet_size -= static_cast<ssize_t>(sizeof(packet.header));

  // Validate header
  if (!ValidateHeader(packet.header)) {
    OLA_WARN << "Invalid pathport packet";
    return 0;
  }

  uint32_t destination = NetworkToHost(packet.header.destination);
  if (destination != m_device_id &&
      destination != PATHPORT_ID_BROADCAST &&
      destination != PATHPORT_STATUS_GROUP &&
      destination != PATHPORT_CONFIG_GROUP &&
      destination != PATHPORT_DATA_GROUP) {
    OLA_WARN << "pathport destination not set to us: " << destination;
    return 0;
  }

  // TODO(simon): Handle multiple pdus here
  pathport_packet_pdu *pdu =
    reinterpret_cast<pathport_packet_pdu*>(packet.data);

  if (packet_size < sizeof(pathport_pdu_header)) {
    OLA_WARN << "Pathport packet too small to fit a pdu header";
    return 0;
  }
  packet_size -= sizeof(pathport_pdu_header);

  switch (NetworkToHost(pdu->head.type)) {
    case PATHPORT_DATA:
      HandleDmxData(pdu->d.data, packet_size);
      break;
    case PATHPORT_ARP_REQUEST:
      SendArpReply();
      break;
    case PATHPORT_ARP_REPLY:
      OLA_DEBUG << "Got pathport arp reply";
      break;
    default:
      OLA_INFO << "Unhandled pathport packet with id: " <<
        NetworkToHost(pdu->head.type);
  }
  return 0;
}


/*
 * Set the closure to be called when we receive data for this universe.
 * @param universe the universe to register the handler for
 * @param handler the Closure to call when there is data for this universe.
 * Ownership of the closure is transferred to the node.
 */
bool PathportNode::SetHandler(uint8_t universe,
                             DmxBuffer *buffer,
                             Closure *closure) {
  if (!closure)
    return false;

  universe_handlers::iterator iter = m_handlers.find(universe);

  if (iter == m_handlers.end()) {
    universe_handler handler;
    handler.buffer = buffer;
    handler.closure = closure;
    m_handlers[universe] = handler;
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
bool PathportNode::RemoveHandler(uint8_t universe) {
  universe_handlers::iterator iter = m_handlers.find(universe);

  if (iter != m_handlers.end()) {
    Closure *old_closure = iter->second.closure;
    m_handlers.erase(iter);
    delete old_closure;
    return true;
  }
  return false;
}


/*
 * Send an arp reply
 */
bool PathportNode::SendArpReply() {
  if (!m_running)
    return false;

  pathport_packet_s packet;
  pathport_packet_pdu *pdu =
    reinterpret_cast<pathport_packet_pdu*>(packet.data);

  // Should this go to status or config?
  PopulateHeader(&packet.header, PATHPORT_STATUS_GROUP);
  pdu->head.type = HostToNetwork((uint16_t) PATHPORT_ARP_REPLY);
  pdu->head.len = HostToNetwork((uint16_t) sizeof(pathport_pdu_arp_reply));

  pdu->d.arp_reply.id = HostToNetwork(m_device_id);
  pdu->d.arp_reply.ip = m_interface.ip_address.s_addr;
  pdu->d.arp_reply.manufacturer_code = NODE_MANUF_ZP_TECH;
  pdu->d.arp_reply.device_class = NODE_CLASS_DMX_NODE;
  pdu->d.arp_reply.device_type = NODE_DEVICE_PATHPORT;
  pdu->d.arp_reply.component_count = 1;

  unsigned int length = sizeof(pathport_packet_header) +
                        sizeof(pathport_pdu_header) +
                        sizeof(pathport_pdu_arp_reply);
  return SendPacket(packet, length, m_config_addr);
}


/*
 * Send some DMX data
 * @param buffer the DMX data
 * @return true if it was send successfully, false otherwise
 */
bool PathportNode::SendDMX(unsigned int universe, const DmxBuffer &buffer) {
  if (!m_running)
    return false;

  if (universe > MAX_UNIVERSES) {
    OLA_WARN << "attempt to send to universe " << universe;
    return false;
  }

  pathport_packet_s packet;
  pathport_packet_pdu *pdu =
    reinterpret_cast<pathport_packet_pdu*>(packet.data);

  // pad to a multiple of 4 bytes
  unsigned int padded_size = (buffer.Size() + 3) & ~3;
  PopulateHeader(&packet.header, PATHPORT_DATA_GROUP);
  pdu->head.type = HostToNetwork((uint16_t) PATHPORT_DATA);
  pdu->head.len = HostToNetwork(
      (uint16_t) (padded_size + sizeof(pathport_pdu_data)));

  pdu->d.data.type = HostToNetwork((uint16_t) XDMX_DATA_FLAT);
  pdu->d.data.channel_count = HostToNetwork((uint16_t) buffer.Size());
  pdu->d.data.universe = 0;
  pdu->d.data.start_code = 0;
  pdu->d.data.offset = HostToNetwork(
      (uint16_t) (DMX_UNIVERSE_SIZE * universe));

  unsigned int length = padded_size;
  buffer.Get(pdu->d.data.data, &length);

  // pad data to multiple of 4 bytes
  length = sizeof(pathport_packet_header) +
           sizeof(pathport_pdu_header) +
           sizeof(pathport_pdu_data) + padded_size;

  return SendPacket(packet, length, m_data_addr);
}


/*
 * Setup the networking compoents.
 */
bool PathportNode::InitNetwork() {
  if (!m_socket.Init()) {
    OLA_WARN << "Socket init failed";
    return false;
  }

  if (!m_socket.Bind(PATHPORT_PORT)) {
    OLA_WARN << "Failed to bind to:" << PATHPORT_PORT;
    m_socket.Close();
    return false;
  }

  if (!m_socket.JoinMulticast(m_interface.ip_address, m_config_addr)) {
      OLA_WARN << "Failed to join multicast to: " << inet_ntoa(m_config_addr);
    m_socket.Close();
    return false;
  }

  if (!m_socket.JoinMulticast(m_interface.ip_address, m_data_addr)) {
      OLA_WARN << "Failed to join multicast to: " << inet_ntoa(m_data_addr);
    m_socket.Close();
    return false;
  }

  if (!m_socket.JoinMulticast(m_interface.ip_address, m_status_addr)) {
      OLA_WARN << "Failed to join multicast to: " << inet_ntoa(m_status_addr);
    m_socket.Close();
    return false;
  }

  m_socket.SetOnData(
    NewClosure(this, &PathportNode::SocketReady, &m_socket));
  return true;
}


/*
 * Fill in a pathport header structure
 */
void PathportNode::PopulateHeader(pathport_packet_header *header,
                                  uint32_t destination) {
  header->protocol = HostToNetwork(PATHPORT_PROTOCOL);
  header->version_major = MAJOR_VERSION;
  header->version_minor = MINOR_VERSION;
  header->sequence = HostToNetwork(m_sequence_number);
  memset(header->reserved, 0, sizeof(header->reserved));
  header->source = HostToNetwork(m_device_id);
  header->destination = HostToNetwork(destination);
}


/*
 * Check a pathport header structure is valid.
 */
bool PathportNode::ValidateHeader(const pathport_packet_header &header) {
  return (
      header.protocol == HostToNetwork(PATHPORT_PROTOCOL) &&
      header.version_major == MAJOR_VERSION &&
      header.version_minor == MINOR_VERSION);
}


/*
 * Handle new DMX data
 */
void PathportNode::HandleDmxData(const pathport_pdu_data &packet,
                                 unsigned int size) {

  if (size < sizeof(pathport_pdu_data)) {
    OLA_WARN << "Small pathport data packet received, ignoring";
    return;
  }

  // Don't handle release messages yet
  if (NetworkToHost(packet.type) != XDMX_DATA_FLAT)
    return;

  if (packet.start_code) {
    OLA_INFO << "Non-0 start code packet received, ignoring";
    return;
  }

  unsigned int offset = NetworkToHost(packet.offset) % DMX_UNIVERSE_SIZE;
  unsigned int universe = NetworkToHost(packet.offset) / DMX_UNIVERSE_SIZE;
  const uint8_t *dmx_data = packet.data;
  unsigned int data_size = std::min(
      NetworkToHost(packet.channel_count),
      (uint16_t) (size - sizeof(pathport_pdu_data)));

  while (data_size > 0 && universe <= MAX_UNIVERSES) {
    unsigned int channels_for_this_universe =
      std::min(data_size, DMX_UNIVERSE_SIZE - offset);

    universe_handlers::iterator iter = m_handlers.find(universe);
    if (iter != m_handlers.end()) {
      iter->second.buffer->SetRange(offset,
                                    dmx_data,
                                    channels_for_this_universe);
      iter->second.closure->Run();
    }
    data_size -= channels_for_this_universe;
    dmx_data += channels_for_this_universe;
    offset = 0;
    universe++;
  }
}


/*
 * @param destination the destination to target
 */
bool PathportNode::SendArpRequest(uint32_t destination) {
  if (!m_running)
    return false;

  pathport_packet_s packet;
  pathport_packet_pdu *pdu =
    reinterpret_cast<pathport_packet_pdu*>(packet.data);

  PopulateHeader(&packet.header, destination);
  pdu->head.type = HostToNetwork((uint16_t) PATHPORT_ARP_REQUEST);
  pdu->head.len = 0;

  unsigned int length = sizeof(pathport_packet_header) +
                        sizeof(pathport_pdu_header);
  return SendPacket(packet, length, m_status_addr);
}


/*
 * Send a packet
 */
bool PathportNode::SendPacket(const pathport_packet_s &packet,
                              unsigned int size,
                              struct in_addr dest) {
  struct sockaddr_in destination;
  destination.sin_family = AF_INET;
  destination.sin_port = HostToNetwork(PATHPORT_PORT);
  destination.sin_addr = dest;

  ssize_t bytes_sent = m_socket.SendTo(
      reinterpret_cast<const uint8_t*>(&packet),
      size,
      destination);

  if (bytes_sent != static_cast<ssize_t>(size)) {
    OLA_WARN << "Only sent " << bytes_sent << " of " << size;
    return false;
  }
  return true;
}
}  // pothport
}  // plugin
}  // ola
