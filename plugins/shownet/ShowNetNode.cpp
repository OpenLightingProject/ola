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
 * ShowNetNode.cpp
 * A ShowNet node
 * Copyright (C) 2005 Simon Newton
 */

#include <string.h>
#include <algorithm>
#include <map>
#include <string>

#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "ola/stl/STLUtils.h"
#include "ola/strings/Utils.h"
#include "plugins/shownet/ShowNetNode.h"


namespace ola {
namespace plugin {
namespace shownet {

using std::string;
using std::map;
using ola::network::HostToLittleEndian;
using ola::network::HostToNetwork;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::LittleEndianToHost;
using ola::network::NetworkToHost;
using ola::network::UDPSocket;
using ola::Callback0;


/*
 * Create a new node
 * @param ip_address the IP address to prefer to listen on, if NULL we choose
 * one.
 */
ShowNetNode::ShowNetNode(const std::string &ip_address)
    : m_running(false),
      m_packet_count(0),
      m_node_name(),
      m_preferred_ip(ip_address),
      m_socket(NULL) {
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

  if (!InitNetwork()) {
    return false;
  }

  m_running = true;
  return true;
}


/*
 * Stop this node
 */
bool ShowNetNode::Stop() {
  if (!m_running) {
    return false;
  }

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
  if (!m_running) {
    return false;
  }

  if (universe >= SHOWNET_MAX_UNIVERSES) {
    OLA_WARN << "Universe index out of bounds, should be between 0 and"
             << SHOWNET_MAX_UNIVERSES << "), was " << universe;
    return false;
  }

  shownet_packet packet;
  unsigned int size = BuildCompressedPacket(&packet, universe, buffer);
  unsigned int bytes_sent = m_socket->SendTo(
      reinterpret_cast<uint8_t*>(&packet),
      size,
      IPV4SocketAddress(m_interface.bcast_address, SHOWNET_PORT));

  if (bytes_sent != size) {
    OLA_WARN << "Only sent " << bytes_sent << " of " << size;
    return false;
  }

  m_packet_count++;
  return true;
}


/*
 * Set the closure to be called when we receive data for this universe.
 * @param universe the universe to register the handler for
 * @param handler the Callback0 to call when there is data for this universe.
 * Ownership of the closure is transferred to the node.
 */
bool ShowNetNode::SetHandler(unsigned int universe,
                             DmxBuffer *buffer,
                             Callback0<void> *closure) {
  if (!closure) {
    return false;
  }

  map<unsigned int, universe_handler>::iterator iter =
      m_handlers.find(universe);

  if (iter == m_handlers.end()) {
    universe_handler handler;
    handler.buffer = buffer;
    handler.closure = closure;
    m_handlers[universe] = handler;
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
bool ShowNetNode::RemoveHandler(unsigned int universe) {
  map<unsigned int, universe_handler>::iterator iter =
      m_handlers.find(universe);

  if (iter != m_handlers.end()) {
    Callback0<void> *old_closure = iter->second.closure;
    m_handlers.erase(iter);
    delete old_closure;
    return true;
  }
  return false;
}


/*
 * Called when there is data on this socket
 */
void ShowNetNode::SocketReady() {
  shownet_packet packet;
  ssize_t packet_size = sizeof(packet);
  ola::network::IPV4SocketAddress source;

  if (!m_socket->RecvFrom(reinterpret_cast<uint8_t*>(&packet),
                          &packet_size, &source)) {
    return;
  }

  // skip packets sent by us
  if (source.Host() != m_interface.ip_address) {
    HandlePacket(&packet, packet_size);
  }
}


/*
 * Handle a shownet packet
 */
bool ShowNetNode::HandlePacket(const shownet_packet *packet,
                               unsigned int packet_size) {
  unsigned int header_size = sizeof(*packet) - sizeof(packet->data);

  if (packet_size <= header_size) {
    OLA_WARN << "Skipping small shownet packet received, size=" << packet_size;
    return false;
  }

  if (NetworkToHost(packet->type) != COMPRESSED_DMX_PACKET) {
    OLA_INFO << "Skipping a packet that isn't a compressed shownet packet";
    return false;
  }

  const shownet_compressed_dmx *dmx_packet = &packet->data.compressed_dmx;
  return HandleCompressedPacket(dmx_packet, packet_size - header_size);
}

bool ShowNetNode::HandleCompressedPacket(const shownet_compressed_dmx *packet,
                                         unsigned int packet_size) {
  uint16_t index_block = LittleEndianToHost(packet->indexBlock[0]);
  if (index_block < MAGIC_INDEX_OFFSET) {
    OLA_WARN << "Strange ShowNet packet, indexBlock[0] is "
             << index_block << ", please contact the developers!";
    return false;
  }

  uint16_t net_slot = LittleEndianToHost(packet->netSlot[0]);

  // We only handle data from the first slot
  // enc_length is the size of the received (optionally encoded) DMX data
  int enc_len = LittleEndianToHost(packet->indexBlock[1]) - index_block;
  if (enc_len < 1 || net_slot == 0) {
    OLA_WARN << "Invalid shownet packet, enc_len=" << enc_len << ", netSlot="
             << net_slot;
    return false;
  }

  // the offset into packet.data of the actual data
  unsigned int data_offset = index_block - MAGIC_INDEX_OFFSET;
  unsigned int received_data_size = packet_size - (
      sizeof(packet) - SHOWNET_COMPRESSED_DATA_LENGTH);

  if (data_offset + enc_len > received_data_size) {
    OLA_WARN << "Not enough shownet data: offset=" << data_offset
             << ", enc_len=" << enc_len << ", received_bytes="
             << received_data_size;
    return false;
  }

  uint16_t slot_size = LittleEndianToHost(packet->slotSize[0]);
  if (!slot_size) {
    OLA_WARN << "Malformed shownet packet, slotSize=" << slot_size;
    return false;
  }

  unsigned int start_channel = (net_slot - 1) % DMX_UNIVERSE_SIZE;
  unsigned int universe_id = (net_slot - 1) / DMX_UNIVERSE_SIZE;

  universe_handler *handler = STLFind(&m_handlers, universe_id);
  if (!handler) {
    OLA_DEBUG << "Not interested in universe " << universe_id
              << ", skipping ";
    return false;
  }

  if (slot_size != enc_len) {
    m_encoder.Decode(start_channel, packet->data + data_offset,
                     enc_len, handler->buffer);
  } else {
    handler->buffer->SetRange(start_channel,
                              packet->data + data_offset,
                              enc_len);
  }
  handler->closure->Run();
  return true;
}


/*
 * Populate a shownet data packet
 */
unsigned int ShowNetNode::BuildCompressedPacket(shownet_packet *packet,
                                                unsigned int universe,
                                                const DmxBuffer &buffer) {
  memset(packet, 0, sizeof(*packet));
  packet->type = HostToNetwork(static_cast<uint16_t>(COMPRESSED_DMX_PACKET));
  memcpy(packet->ip, &m_interface.ip_address, sizeof(packet->ip));

  shownet_compressed_dmx *compressed_dmx = &packet->data.compressed_dmx;

  compressed_dmx->netSlot[0] = HostToLittleEndian(
      static_cast<uint16_t>(universe * DMX_UNIVERSE_SIZE + 1));
  compressed_dmx->slotSize[0] = HostToLittleEndian(
      static_cast<uint16_t>(buffer.Size()));

  unsigned int enc_len = sizeof(packet->data);
  if (!m_encoder.Encode(buffer, compressed_dmx->data, &enc_len)) {
    OLA_WARN << "Failed to encode all data (used " << enc_len << " bytes";
  }

  compressed_dmx->indexBlock[0] = HostToLittleEndian(
      static_cast<uint16_t>(MAGIC_INDEX_OFFSET));
  compressed_dmx->indexBlock[1] = HostToLittleEndian(
      static_cast<uint16_t>(MAGIC_INDEX_OFFSET + enc_len));

  compressed_dmx->sequence = HostToNetwork(m_packet_count);

  strings::CopyToFixedLengthBuffer(m_node_name, compressed_dmx->name,
                                   arraysize(compressed_dmx->name));
  return (sizeof(*packet) - sizeof(packet->data)) +
         (sizeof(*compressed_dmx) - SHOWNET_COMPRESSED_DATA_LENGTH + enc_len);
}


/*
 * Setup the networking components.
 */
bool ShowNetNode::InitNetwork() {
  m_socket = new UDPSocket();

  if (!m_socket->Init()) {
    OLA_WARN << "Socket init failed";
    delete m_socket;
    return false;
  }

  if (!m_socket->Bind(IPV4SocketAddress(IPV4Address::WildCard(),
                                        SHOWNET_PORT))) {
    delete m_socket;
    return false;
  }

  if (!m_socket->EnableBroadcast()) {
    OLA_WARN << "Failed to enable broadcasting";
    delete m_socket;
    return false;
  }

  m_socket->SetOnData(NewCallback(this, &ShowNetNode::SocketReady));
  return true;
}
}  // namespace shownet
}  // namespace plugin
}  // namespace ola
