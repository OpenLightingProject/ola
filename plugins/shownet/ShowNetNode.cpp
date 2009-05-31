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

#include <lla/Logging.h>
#include "RunLengthEncoder.h"
#include "ShowNetNode.h"
#include "ShowNetPackets.h"

namespace lla {
namespace shownet {

using std::string;
using std::map;
using lla::network::UdpSocket;
using lla::Closure;


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
    m_encoder = new RunLengthEncoder();
    memset(&m_destination, 0, sizeof(m_destination));
}


/*
 * Cleanup
 */
ShowNetNode::~ShowNetNode() {
  Stop();
  delete m_encoder;
}


/*
 * Start this node
 */
bool ShowNetNode::Start() {
  if (m_running)
    return false;

  if (!m_interface_picker.ChooseInterface(m_interface, m_preferred_ip)) {
    LLA_INFO << "Failed to find an interface";
    return false;
  }

  if (!InitNetwork())
    return false;

  m_destination.sin_family = AF_INET;
  m_destination.sin_port = htons(SHOWNET_PORT);
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
                          const lla::DmxBuffer &buffer) {

  if (!m_running)
    return false;

  if (universe >= SHOWNET_MAX_UNIVERSES) {
    LLA_WARN << "Universe index out of bounds, should be between 0 and" <<
                  SHOWNET_MAX_UNIVERSES << "), was " << universe;
    return false;
  }

  shownet_data_packet packet;
  memset(&packet.data, 0, sizeof(packet.data));

  // setup the fields in the shownet packet
  packet.sigHi = SHOWNET_ID_HIGH;
  packet.sigLo = SHOWNET_ID_LOW;
  memcpy(packet.ip, &m_interface, sizeof(packet.ip));

  packet.netSlot[0] = (universe * DMX_UNIVERSE_SIZE) + 1;
  packet.netSlot[1] = 0;
  packet.netSlot[2] = 0;
  packet.netSlot[3] = 0;
  packet.slotSize[0] = buffer.Size();
  packet.slotSize[1] = 0;
  packet.slotSize[2] = 0;
  packet.slotSize[3] = 0;

  unsigned int enc_len = sizeof(packet.data);
  if (!m_encoder->Encode(buffer, packet.data, enc_len))
    LLA_WARN << "Failed to encode all data (used " << enc_len << " bytes";

  packet.indexBlock[0] = 0x0b;
  packet.indexBlock[1] = 0x0b + enc_len;
  packet.indexBlock[2] = 0;
  packet.indexBlock[3] = 0;
  packet.indexBlock[4] = 0;

  packet.packetCountHi = ShortGetHigh(m_packet_count);
  packet.packetCountLo = ShortGetLow(m_packet_count);

  // magic numbers - not sure what these do
  packet.block[2] = 0x58;
  packet.block[3] = 0x02;
  strncpy((char*) packet.name, m_node_name.data(), SHOWNET_NAME_LENGTH);

  ssize_t bytes_sent = m_socket->SendTo((uint8_t*) &packet,
                                        sizeof(packet),
                                        m_destination);

  if (bytes_sent != sizeof(packet)) {
    LLA_WARN << "Only sent " << bytes_sent << " of " << sizeof(packet);
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

  if (iter == m_handlers.end())
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
 * Remove the handled for this universe
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
  memset(&packet, 0, sizeof(packet));
  unsigned int header_size = sizeof(packet) - sizeof(packet.data);

  //read here
  ssize_t packet_size = sizeof(packet);
  m_socket->RecvFrom((uint8_t*) &packet, packet_size);

  if (packet_size < header_size) {
    LLA_WARN << "Small shownet packet received, discarding";
    return -1;
  }

  unsigned int data_size = packet_size - header_size;

  if (packet.sigHi != SHOWNET_ID_HIGH || packet.sigLo == SHOWNET_ID_LOW)
    return -1; // not a shownet packet

  // We only handle data from the first slot
  int enc_len = packet.indexBlock[1] - packet.indexBlock[0];
  if (enc_len < 1 || packet.netSlot[0] == 0) {
    LLA_WARN << "Invalid shownet packet";
    return -1;
  }
  if (packet.indexBlock[1] >= data_size) {
    LLA_WARN << "data offset is too large";
    return -1;
  }

  unsigned int universe_id = (packet.netSlot[0] - 1) / DMX_UNIVERSE_SIZE;
  map<unsigned int, universe_handler>::iterator iter =
    m_handlers.find(universe_id);

  if (iter == m_handlers.end()) {
    LLA_DEBUG << "Not interested in universe " << universe_id <<
      ", skipping ";
    return -1;
  }

  unsigned int slot_len = packet.slotSize[0];
  int start_channel = (packet.netSlot[0] - 1) % DMX_UNIVERSE_SIZE;

  if (slot_len != enc_len) {
    m_encoder->Decode(iter->second.buffer,
                      start_channel,
                      packet.data + packet.indexBlock[0],
                      enc_len);
  } else {
    iter->second.buffer.SetRange(start_channel,
                                 packet.data + packet.indexBlock[0],
                                 enc_len);
  }
  iter->second.closure->Run();
  return 0;
}


/*
 * Setup the networking compoents.
 */
bool ShowNetNode::InitNetwork() {
  m_socket = new UdpSocket();

  if (!m_socket->Init()) {
    LLA_WARN << "Socket init failed";
    delete m_socket;
    return false;
  }

  if (!m_socket->Bind(SHOWNET_PORT)) {
    LLA_WARN << "Failed to bind to:" << SHOWNET_PORT;
    delete m_socket;
    return false;
  }

  if (!m_socket->EnableBroadcast()) {
    LLA_WARN << "Failed to enable broadcasting";
    delete m_socket;
    return false;
  }

  return true;
}

} //shownet
} //lla
