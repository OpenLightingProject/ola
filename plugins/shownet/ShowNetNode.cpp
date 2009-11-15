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
 * ShowNetNode.cpp
 * A ShowNet node
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string.h>
#include <algorithm>
#include <ola/Logging.h>
#include <ola/network/NetworkUtils.h>
#include "ShowNetNode.h"


namespace ola {
namespace shownet {

using std::string;
using std::map;
using ola::network::UdpSocket;
using ola::network::HostToNetwork;
using ola::Closure;


/*
 * Create a new node
 * @param ip_address the IP address to prefer to listen on, if NULL we choose
 * one.
 */
ShowNetNode::ShowNetNode(const string &ip_address):
  m_running(false),
  m_packet_count(0),
  m_node_name(),
  m_preferred_ip(ip_address) {
    memset(&m_destination, 0, sizeof(m_destination));
}


/*
 * Cleanup
 */
ShowNetNode::~ShowNetNode() {
  Stop();

  std::map<unsigned int, universe_handler>::iterator iter;
  for (iter = m_handlers.begin(); iter != m_handlers.end(); ++iter) {
    delete iter->second.closure;
  }
  m_handlers.clear();
}


/*
 * Start this node
 */
bool ShowNetNode::Start() {
  if (m_running)
    return false;

  if (!m_interface_picker.ChooseInterface(&m_interface, m_preferred_ip)) {
    OLA_INFO << "Failed to find an interface";
    return false;
  }

  if (!InitNetwork())
    return false;

  m_destination.sin_family = AF_INET;
  m_destination.sin_port = HostToNetwork(SHOWNET_PORT);
  m_destination.sin_addr = m_interface.bcast_address;
  m_running = true;
  return true;
}


/*
 * Stop this node
 */
bool ShowNetNode::Stop() {
  if (!m_running)
    return false;

  if (m_socket) {
    delete m_socket;
    m_socket = NULL;
  }

  m_running = false;
  return true;
}


/*
 * Set the node name
 * @param name the new node name
 */
void ShowNetNode::SetName(const string &name) {
  m_node_name = name;
}


/*
 * Send some DMX data
 * @param universe the id of the universe to send
 * @param buffer the DMX data
 * @return true if it was send successfully, false otherwise
 */
bool ShowNetNode::SendDMX(unsigned int universe,
                          const ola::DmxBuffer &buffer) {

  if (!m_running)
    return false;

  if (universe >= SHOWNET_MAX_UNIVERSES) {
    OLA_WARN << "Universe index out of bounds, should be between 0 and" <<
                  SHOWNET_MAX_UNIVERSES << "), was " << universe;
    return false;
  }

  shownet_data_packet packet;
  unsigned int size = PopulatePacket(packet, universe, buffer);
  ssize_t bytes_sent = m_socket->SendTo((uint8_t*) &packet,
                                        size,
                                        m_destination);

  if (bytes_sent != size) {
    OLA_WARN << "Only sent " << bytes_sent << " of " << size;
    return false;
  }

  m_packet_count++;
  return true;
}


/*
 * Get the DMX data for this universe.
 */
DmxBuffer ShowNetNode::GetDMX(unsigned int universe) {
  map<unsigned int, universe_handler>::const_iterator iter =
    m_handlers.find(universe);

  if (iter != m_handlers.end())
    return iter->second.buffer;
  else {
    DmxBuffer buffer;
    return buffer;
  }
}


/*
 * Set the closure to be called when we receive data for this universe.
 * @param universe the universe to register the handler for
 * @param handler the Closure to call when there is data for this universe.
 * Ownership of the closure is transferred to the node.
 */
bool ShowNetNode::SetHandler(unsigned int universe, Closure *closure) {
  if (!closure)
    return false;

  map<unsigned int, universe_handler>::iterator iter =
    m_handlers.find(universe);

  if (iter == m_handlers.end()) {
    universe_handler handler;
    handler.closure = closure;
    handler.buffer.Blackout();
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
bool ShowNetNode::RemoveHandler(unsigned int universe) {
  map<unsigned int, universe_handler>::iterator iter =
    m_handlers.find(universe);

  if (iter != m_handlers.end()) {
    Closure *old_closure = iter->second.closure;
    m_handlers.erase(iter);
    delete old_closure;
    return true;
  }
  return false;
}


/*
 * Called when there is data on this socket
 */
int ShowNetNode::SocketReady() {
  shownet_data_packet packet;
  ssize_t packet_size = sizeof(packet);
  struct sockaddr_in source;
  socklen_t source_length = sizeof(source);

  if(!m_socket->RecvFrom((uint8_t*) &packet, &packet_size, source,
                          source_length))
    return -1;

  // skip packets sent by us
  if (source.sin_addr.s_addr == m_interface.ip_address.s_addr)
    return 0;

  return !HandlePacket(packet, packet_size);
}


/*
 * Handle a shownet packet
 */
bool ShowNetNode::HandlePacket(const shownet_data_packet &packet,
                               unsigned int packet_size) {

  unsigned int header_size = sizeof(packet) - sizeof(packet.data);

  if (packet_size <= header_size) {
    OLA_WARN << "Skipping small shownet packet received, size=" << packet_size;
    return false;
  }

  if (packet.sigHi != SHOWNET_ID_HIGH || packet.sigLo != SHOWNET_ID_LOW) {
    OLA_INFO << "Skipping a packet that isn't shownet";
    return false;
  }

  if (packet.indexBlock[0] < MAGIC_INDEX_OFFSET) {
    OLA_WARN << "Strange ShowNet packet, indexBlock[0] is " <<
      packet.indexBlock[0] << ", please contact the developers!";
    return false;
  }

  // We only handle data from the first slot
  // enc_length is the size of the received (optionally encoded) DMX data
  int enc_len = packet.indexBlock[1] - packet.indexBlock[0];
  if (enc_len < 1 || packet.netSlot[0] == 0) {
    OLA_WARN << "Invalid shownet packet, enc_len=" << enc_len << ", netSlot="
      << packet.netSlot[0];
    return false;
  }

  // the offset into packet.data of the actual data
  unsigned int data_offset = packet.indexBlock[0] - MAGIC_INDEX_OFFSET;
  unsigned int received_data_size = packet_size - header_size;

  if (data_offset + enc_len > received_data_size) {
    OLA_WARN << "Not enough shownet data: offset=" << data_offset <<
      ", enc_len=" << enc_len << ", received_bytes=" << received_data_size;
    return false;
  }

  if (!packet.slotSize[0]) {
    OLA_WARN << "Malformed shownet packet, slotSize=" << packet.slotSize[0];
    return false;
  }

  unsigned int start_channel = (packet.netSlot[0] - 1) % DMX_UNIVERSE_SIZE;
  unsigned int universe_id = (packet.netSlot[0] - 1) / DMX_UNIVERSE_SIZE;
  map<unsigned int, universe_handler>::iterator iter =
    m_handlers.find(universe_id);

  if (iter == m_handlers.end()) {
    OLA_DEBUG << "Not interested in universe " << universe_id <<
      ", skipping ";
    return false;
  }

  if (packet.slotSize[0] != enc_len) {
    m_encoder.Decode(&iter->second.buffer,
                     start_channel,
                     packet.data + data_offset,
                     enc_len);
  } else {
    iter->second.buffer.SetRange(start_channel,
                                 packet.data + data_offset,
                                 enc_len);
  }
  iter->second.closure->Run();
  return true;

}


/*
 * Populate a shownet data packet
 */
unsigned int ShowNetNode::PopulatePacket(shownet_data_packet &packet,
                                         unsigned int universe,
                                         const DmxBuffer &buffer) {

  memset(&packet, 0, sizeof(packet));

  // setup the fields in the shownet packet
  packet.sigHi = SHOWNET_ID_HIGH;
  packet.sigLo = SHOWNET_ID_LOW;
  memcpy(packet.ip, &m_interface.ip_address, sizeof(packet.ip));

  packet.netSlot[0] = (universe * DMX_UNIVERSE_SIZE) + 1;
  packet.slotSize[0] = buffer.Size();

  unsigned int enc_len = sizeof(packet.data);
  if (!m_encoder.Encode(buffer, packet.data, enc_len))
    OLA_WARN << "Failed to encode all data (used " << enc_len << " bytes";

  packet.indexBlock[0] = MAGIC_INDEX_OFFSET;
  packet.indexBlock[1] = MAGIC_INDEX_OFFSET + enc_len;

  packet.packetCountHi = ShortGetHigh(m_packet_count);
  packet.packetCountLo = ShortGetLow(m_packet_count);

  strncpy((char*) packet.name, m_node_name.data(), SHOWNET_NAME_LENGTH);
  return sizeof(packet) - sizeof(packet.data) + enc_len;
}


/*
 * Setup the networking compoents.
 */
bool ShowNetNode::InitNetwork() {
  m_socket = new UdpSocket();

  if (!m_socket->Init()) {
    OLA_WARN << "Socket init failed";
    delete m_socket;
    return false;
  }

  if (!m_socket->Bind(SHOWNET_PORT)) {
    OLA_WARN << "Failed to bind to:" << SHOWNET_PORT;
    delete m_socket;
    return false;
  }

  if (!m_socket->EnableBroadcast()) {
    OLA_WARN << "Failed to enable broadcasting";
    delete m_socket;
    return false;
  }

  m_socket->SetOnData(NewClosure(this, &ShowNetNode::SocketReady));
  return true;
}

} //shownet
} //ola
