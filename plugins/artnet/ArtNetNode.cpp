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
 * ArtNetNode.cpp
 * An ArtNet node
 * Copyright (C) 2005-2010 Simon Newton
 */

#include <string.h>
#include <algorithm>
#include <map>
#include <set>
#include <string>

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/artnet/ArtNetNode.h"


namespace ola {
namespace plugin {
namespace artnet {

using ola::Callback1;
using ola::Closure;
using ola::network::HostToLittleEndian;
using ola::network::LittleEndianToHost;
using ola::network::HostToNetwork;
using ola::network::UdpSocket;
using std::pair;
using std::string;


const char ArtNetNode::ARTNET_ID[] = "Art-Net";

/*
 * Create a new node
 * @param ip_address the IP address to prefer to listen on, if NULL we choose
 * one.
 * @param short_name
 * @param long_name
 * @param subnet_address the ArtNet 'subnet' address, 4 bits.
 */
ArtNetNode::ArtNetNode(const string &ip_address,
                       const string &short_name,
                       const string &long_name,
                       const TimeStamp *wake_up_time,
                       uint8_t subnet_address)
    : m_running(false),
      m_send_reply_on_change(true),
      m_short_name(short_name),
      m_long_name(long_name),
      m_broadcast_threshold(BROADCAST_THRESHOLD),
      m_unsolicited_replies(0),
      m_preferred_ip(ip_address),
      m_wake_time(wake_up_time),
      m_socket(NULL) {
  memset(&m_destination, 0, sizeof(m_destination));

  // reset all the port structures
  for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++) {
    m_input_ports[i].universe_address = 0;
    m_input_ports[i].sequence_number = 0;
    m_input_ports[i].enabled = false;

    m_output_ports[i].universe_address = 0;
    m_output_ports[i].sequence_number = 0;
    m_output_ports[i].enabled = false;
    m_output_ports[i].is_merging = false;
    m_output_ports[i].merge_mode = ARTNET_MERGE_HTP;
    m_output_ports[i].buffer = NULL;
    m_output_ports[i].on_data = NULL;
    for (unsigned int j = 0; j < MAX_MERGE_SOURCES; j++)
      m_output_ports[i].sources[j].address.s_addr = 0;
  }

  SetSubnetAddress(subnet_address);
}


/*
 * Cleanup
 */
ArtNetNode::~ArtNetNode() {
  Stop();

  for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++) {
    if (m_output_ports[i].on_data)
      delete m_output_ports[i].on_data;
  }

  /*
  std::map<unsigned int, Callback1<void, RDMCommand*>* >::iterator rdm_iter;
  for (rdm_iter = m_rdm_handlers.begin(); rdm_iter != m_rdm_handlers.end();
       ++rdm_iter) {
    delete rdm_iter->second;
  }
  m_rdm_handlers.clear();
  */
}


/*
 * Start this node. The port modifying functions can be called before this.
 */
bool ArtNetNode::Start() {
  if (m_running)
    return false;

  ola::network::InterfacePicker *picker =
    ola::network::InterfacePicker::NewPicker();
  if (!picker->ChooseInterface(&m_interface, m_preferred_ip)) {
    delete picker;
    OLA_INFO << "Failed to find an interface";
    return false;
  }
  delete picker;

  if (!InitNetwork())
    return false;

  m_destination.sin_family = AF_INET;
  m_destination.sin_port = HostToNetwork(ARTNET_PORT);
  m_destination.sin_addr = m_interface.bcast_address;
  m_running = true;

  MaybeSendPoll();  // send a poll, this will result in us repling as well
  SendPollReply(m_destination);
  return true;
}


/*
 * Stop this node
 */
bool ArtNetNode::Stop() {
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
 * Set the short name
 */
bool ArtNetNode::SetShortName(const string &name) {
  m_short_name = name;
  if (m_running && m_send_reply_on_change) {
    m_unsolicited_replies++;
    return SendPollReply(m_destination);
  }
  return true;
}


/*
 * Set the long name
 */
bool ArtNetNode::SetLongName(const string &name) {
  m_long_name = name;
  if (m_running && m_send_reply_on_change) {
    m_unsolicited_replies++;
    return SendPollReply(m_destination);
  }
  return true;
}


/*
 * The the subnet address for this node
 */
bool ArtNetNode::SetSubnetAddress(uint8_t subnet_address) {
  subnet_address = subnet_address << 4;
  for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++) {
    m_input_ports[i].universe_address = subnet_address |
      (m_input_ports[i].universe_address & 0x0f);
    m_output_ports[i].universe_address = subnet_address |
      (m_output_ports[i].universe_address & 0x0f);
  }

  if (m_running && m_send_reply_on_change) {
    m_unsolicited_replies++;
    return SendPollReply(m_destination);
  }
  return true;
}


/*
 * Set the universe for a port
 */
bool ArtNetNode::SetPortUniverse(artnet_port_type type,
                                 uint8_t port_id,
                                 uint8_t universe_id) {
  if (port_id > ARTNET_MAX_PORTS) {
    OLA_WARN << "Port index of out bounds: " << port_id << " > " <<
      ARTNET_MAX_PORTS;
    return false;
  }

  if (type == ARTNET_INPUT_PORT) {
    m_input_ports[port_id].universe_address = (
        (universe_id & 0x0f) |
        (m_input_ports[port_id].universe_address & 0xf0));

    bool ports_previously_enabled = false;
    for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++)
      ports_previously_enabled |= m_input_ports[i].enabled;

    m_input_ports[port_id].enabled = universe_id != ARTNET_DISABLE_PORT;
    if (!ports_previously_enabled && m_input_ports[port_id].enabled)
      MaybeSendPoll();

  } else {
    m_output_ports[port_id].universe_address = (
        (universe_id & 0x0f) |
        (m_output_ports[port_id].universe_address & 0xf0));
    m_output_ports[port_id].enabled = universe_id != ARTNET_DISABLE_PORT;
  }

  if (m_running && m_send_reply_on_change) {
    m_unsolicited_replies++;
    return SendPollReply(m_destination);
  }
  return true;
}


/*
 * Return the current universe address for a port
 */
uint8_t ArtNetNode::GetPortUniverse(artnet_port_type type, uint8_t port_id) {
  if (port_id > ARTNET_MAX_PORTS) {
    OLA_WARN << "Port index of out bounds: " << port_id << " > " <<
      ARTNET_MAX_PORTS;
    return 0;
  }

  if (type == ARTNET_INPUT_PORT)
    return m_input_ports[port_id].universe_address;
  else
    return m_output_ports[port_id].universe_address;
}


/*
 * Set the merge mode for an output port
 */
bool ArtNetNode::SetMergeMode(uint8_t port_id, artnet_merge_mode merge_mode) {
  if (port_id > ARTNET_MAX_PORTS) {
    OLA_WARN << "Port index of out bounds: " << port_id << " > " <<
      ARTNET_MAX_PORTS;
    return false;
  }

  m_output_ports[port_id].merge_mode = merge_mode;
  if (m_running && m_send_reply_on_change) {
    m_unsolicited_replies++;
    return SendPollReply(m_destination);
  }
  return true;
}


/*
 * Send an ArtPoll if any of the ports are sending data
 */
bool ArtNetNode::MaybeSendPoll() {
  if (!m_running)
    return false;

  bool send = false;
  for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++)
    send |= m_input_ports[i].enabled;

  if (!send)
    return true;

  OLA_DEBUG << "Sending ArtPoll";
  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_POLL);
  memset(&packet.data.poll, 0, sizeof(packet.data.poll));
  packet.data.poll.version[1] = ARTNET_VERSION;
  // send PollReplies when something changes
  packet.data.poll.talk_to_me = 0x02;
  unsigned int size = sizeof(packet.data.poll);

  return SendPacket(packet, size, m_destination);
}


/*
 * Send some DMX data
 * @param universe the id of the universe to send
 * @param buffer the DMX data
 * @return true if it was send successfully, false otherwise
 */
bool ArtNetNode::SendDMX(uint8_t port_id, const DmxBuffer &buffer) {
  if (!m_running)
    return false;

  if (port_id > ARTNET_MAX_PORTS) {
    OLA_WARN << "Port index of out bounds: " << port_id << " > " <<
      ARTNET_MAX_PORTS;
    return false;
  }

  if (!buffer.Size()) {
    OLA_DEBUG << "Not sending 0 length packet";
    return true;
  }

  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_DMX);
  memset(&packet.data.dmx, 0, sizeof(packet.data.dmx));

  packet.data.dmx.version[1] = ARTNET_VERSION;
  packet.data.dmx.sequence = m_input_ports[port_id].sequence_number;
  packet.data.dmx.physical = port_id;
  packet.data.dmx.universe = m_input_ports[port_id].universe_address;
  unsigned int buffer_size = sizeof(packet.data.dmx.data);
  packet.data.dmx.length[0] = buffer_size >> 8;
  packet.data.dmx.length[1] = buffer_size & 0xff;
  buffer.Get(packet.data.dmx.data, &buffer_size);

  // the packet size needs to be a multiple of two, correct here if needed
  if (buffer_size % 2)
    buffer_size++;
  unsigned int size = sizeof(packet.data.dmx) - DMX_UNIVERSE_SIZE + buffer_size;

  bool sent_ok = false;
  if (m_input_ports[port_id].subscribed_nodes.size() >= BROADCAST_THRESHOLD) {
    sent_ok = SendPacket(packet, size, m_destination);
    m_input_ports[port_id].sequence_number++;
  } else {
    struct sockaddr_in destination;
    destination.sin_family = AF_INET;
    destination.sin_port = HostToNetwork(ARTNET_PORT);
    map<IPAddress, TimeStamp>::iterator iter =
      m_input_ports[port_id].subscribed_nodes.begin();

    TimeStamp last_heard_threshold = (
        *m_wake_time - TimeInterval(NODE_TIMEOUT, 0));
    while (iter != m_input_ports[port_id].subscribed_nodes.end()) {
      // if this node has timed out, remove it from the set
      if (iter->second < last_heard_threshold) {
        m_input_ports[port_id].subscribed_nodes.erase(iter++);
        continue;
      }
      destination.sin_addr = iter->first;
      sent_ok |= SendPacket(packet, size, destination);
      ++iter;
    }

    if (!m_input_ports[port_id].subscribed_nodes.size()) {
      OLA_DEBUG <<
        "Suppressing data transmit due to no active nodes for universe" <<
        static_cast<int>(m_input_ports[port_id].universe_address);
      sent_ok = true;
    } else {
      // We sent at least one packet, increment the sequence number
      m_input_ports[port_id].sequence_number++;
    }
  }

  if (!sent_ok)
    OLA_WARN << "Failed to send ArtNet DMX packet";
  return sent_ok;
}


/*
 * Set the closure to be called when we receive data for this universe.
 * @param universe the universe to register the handler for
 * @param handler the Closure to call when there is data for this universe.
 * Ownership of the closure is transferred to the node.
 */
bool ArtNetNode::SetDMXHandler(uint8_t port_id,
                               DmxBuffer *buffer,
                               Closure<void> *on_data) {
  if (port_id > ARTNET_MAX_PORTS) {
    OLA_WARN << "Port index of out bounds: " << port_id << " > " <<
      ARTNET_MAX_PORTS;
    return false;
  }

  if (m_output_ports[port_id].on_data)
    delete m_output_ports[port_id].on_data;
  m_output_ports[port_id].buffer = buffer;
  m_output_ports[port_id].on_data = on_data;
  return true;
}


/*
 * Send an RDMCommand
 * @param universe the id of the universe to send
 * @param command the command to send
 * @return true if it was send successfully, false otherwise
bool ArtNetNode::SendRDM(unsigned int universe, const RDMCommand &command) {
  if (!m_running)
    return false;

  if (universe >= ARTNET_MAX_UNIVERSES) {
    OLA_WARN << "Universe index out of bounds, should be between 0 and" <<
                  ARTNET_MAX_UNIVERSES << "), was " << universe;
    return false;
  }

  artnet_data_packet packet;
  unsigned int size = PopulatePacket(&packet, universe, buffer);
  ssize_t bytes_sent = m_socket->SendTo(reinterpret_cast<uint8_t*>(&packet),
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
 * Set the closure to be called when we receive rdm data for this universe.
 * @param universe the universe to register the handler for
 * @param callback the Callback to invoke when there is data for this universe.
 * Ownership of the closure is transferred to the node.
bool ArtNetNode::SetRDMHandler(unsigned int universe,
                               Callback1<void, RDMCommand*> > *callback) {
  map<unsigned int, Callback1<void, RDMCommand*> > *>::iterator iter =
    m_rdm_handlers.find(universe);

  if (callback) {
    // adding
    if (iter == m_rdm_handlers.end()) {
      m_rdm_handlers[universe] = callback;
    } else {
      delete iter->second;
      iter->second = callback;
    }
  } else if (iter != m_rdm_handlers.end()) {
    // removing
    delete iter->second;
    m_rdm_handlers.erase(iter);
  }
  return true;
}


/*
 * Called when there is data on this socket
 */
void ArtNetNode::SocketReady() {
  artnet_packet packet;
  ssize_t packet_size = sizeof(packet);
  struct sockaddr_in source;
  socklen_t source_length = sizeof(source);

  if (!m_socket->RecvFrom(reinterpret_cast<uint8_t*>(&packet),
                          &packet_size,
                          source,
                          source_length))
    return;

  // skip packets sent by us
  if (source.sin_addr.s_addr != m_interface.ip_address.s_addr)
    HandlePacket(source.sin_addr, packet, packet_size);
}


/*
 * Send an ArtPollReply message
 */
bool ArtNetNode::SendPollReply(struct sockaddr_in destination) {
  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_REPLY);
  memset(&packet.data.reply, 0, sizeof(packet.data.reply));

  memcpy(packet.data.reply.ip,
         &m_interface.ip_address.s_addr,
         sizeof(packet.data.reply.ip));
  packet.data.reply.port = HostToLittleEndian(ARTNET_PORT);
  packet.data.reply.version[1] = ARTNET_VERSION;
  packet.data.reply.subnet_address[1] = m_input_ports[0].universe_address >> 4;
  packet.data.reply.oem = HostToNetwork(OEM_CODE);
  packet.data.reply.status1 = 0xd2;  // normal indicators, rdm enabled
  packet.data.reply.esta_id = HostToLittleEndian(ESTA_CODE);
  strncpy(reinterpret_cast<char*>(packet.data.reply.short_name),
          m_short_name.data(),
          ARTNET_SHORT_NAME_LENGTH);
  strncpy(reinterpret_cast<char*>(packet.data.reply.long_name),
          m_long_name.data(),
          ARTNET_LONG_NAME_LENGTH);

  std::stringstream str;
  str << "#0001 [" << m_unsolicited_replies << "] OLA";
  strncpy(reinterpret_cast<char*>(packet.data.reply.node_report),
          str.str().data(),
          ARTNET_REPORT_LENGTH);
  packet.data.reply.number_ports[1] = ARTNET_MAX_PORTS;
  for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++) {
    packet.data.reply.port_types[i] = 0xc0;  // input and output DMX
    packet.data.reply.good_input[i] = m_input_ports[i].enabled ? 0x0 : 0x8;
    packet.data.reply.sw_in[i] = m_input_ports[i].universe_address;
    packet.data.reply.good_output[i] = (
        (m_output_ports[i].enabled ? 0x80 : 0x00) |
        (m_output_ports[i].merge_mode == ARTNET_MERGE_LTP ? 0x2 : 0x0) |
        (m_output_ports[i].is_merging ? 0x8 : 0x0));
    packet.data.reply.sw_out[i] = m_output_ports[i].universe_address;
  }
  packet.data.reply.style = NODE_CODE;
  memcpy(packet.data.reply.mac,
         m_interface.hw_address,
         ola::network::MAC_LENGTH);
  memcpy(packet.data.reply.bind_ip,
         &m_interface.ip_address.s_addr,
         sizeof(packet.data.reply.bind_ip));
  // maybe set status2 here if the web UI is enabled
  if (!SendPacket(packet, sizeof(packet.data.reply), destination)) {
    OLA_INFO << "Failed to send ArtPollReply";
    return false;
  }
  return true;
}


/*
 * Handle a artnet packet
 */
void ArtNetNode::HandlePacket(IPAddress source_address,
                              const artnet_packet &packet,
                              unsigned int packet_size) {
  unsigned int header_size = sizeof(packet) - sizeof(packet.data);

  if (packet_size <= header_size) {
    OLA_WARN << "Skipping small artnet packet received, size=" << packet_size;
    return;
  }

  switch (LittleEndianToHost(packet.op_code)) {
    case ARTNET_POLL:
      HandlePollPacket(source_address,
                       packet.data.poll,
                       packet_size - header_size);
      break;
    case ARTNET_REPLY:
      HandleReplyPacket(source_address,
                        packet.data.reply,
                        packet_size - header_size);
      break;
    case ARTNET_DMX:
      HandleDataPacket(source_address,
                       packet.data.dmx,
                       packet_size - header_size);
      break;
    default:
      OLA_INFO << "ArtNet got unknown packet " << std::hex <<
        LittleEndianToHost(packet.op_code);
  }
}


/*
 * Handle an ArtPoll packet
 */
void ArtNetNode::HandlePollPacket(IPAddress source_address,
                                  const artnet_poll_t &packet,
                                  unsigned int packet_size) {
  if (packet_size < sizeof(artnet_poll_t)) {
    OLA_INFO << "ArtPoll too small, was " << packet_size << " required " <<
      sizeof(artnet_poll_t);
    return;
  }

  if (packet.version[0] || packet.version[1] != ARTNET_VERSION) {
    OLA_INFO << "ArtPoll version mismatch, was " <<
      static_cast<int>(packet.version[0]) <<
      static_cast<int>(packet.version[1]);
    return;
  }

  m_send_reply_on_change = packet.talk_to_me & 0x02;
  // It's unclear if this should be broadcast or unicast, stick with broadcast
  SendPollReply(m_destination);
  (void) source_address;
}


/*
 * Handle an ArtPollReply packet
 */
void ArtNetNode::HandleReplyPacket(IPAddress source_address,
                                   const artnet_reply_t &packet,
                                   unsigned int packet_size) {
  // older versions don't have the bind_ip and the extra filler, make sure we
  // support these
  unsigned int minimum_reply_size = (
      sizeof(packet) -
      sizeof(packet.filler) -
      sizeof(packet.status2) -
      sizeof(packet.bind_index) -
      sizeof(packet.bind_ip));
  if (packet_size < minimum_reply_size) {
    OLA_INFO << "ArtPollReply too small, was " << packet_size << " required "
      << minimum_reply_size;
    return;
  }

  if (packet.version[0] || packet.version[1] != ARTNET_VERSION) {
    OLA_INFO << "ArtPollReply version mismatch, was " <<
      static_cast<int>(packet.version[0]) <<
      static_cast<int>(packet.version[1]) << " from " <<
      inet_ntoa(source_address);
    // TODO(simon): this is commented out because libartnet has a bug where it
    // sets the version to 0
    // return;
  }

  // Update the subscribed nodes list
  unsigned int port_limit = std::min((uint8_t) ARTNET_MAX_PORTS,
                                     packet.number_ports[1]);
  for (unsigned int i = 0; i < port_limit; i++) {
    if (packet.port_types[i] & 0x80) {
      // port is of type output
      uint8_t universe_id = packet.sw_out[i];
      for (unsigned int port_id = 0; port_id < ARTNET_MAX_PORTS; port_id++) {
        if (m_input_ports[port_id].enabled &&
            m_input_ports[port_id].universe_address == universe_id) {
          m_input_ports[port_id].subscribed_nodes[source_address] =
            *m_wake_time;
        }
      }
    }
  }
}


/*
 * Handle a DMX Data packet, this takes care of the merging
 */
void ArtNetNode::HandleDataPacket(IPAddress source_address,
                                  const artnet_dmx_t &packet,
                                  unsigned int packet_size) {
  // The data section needs to be at least 2 bytes according to the spec
  unsigned int header_size = sizeof(artnet_dmx_t) - DMX_UNIVERSE_SIZE;
  if (packet_size < header_size + 2) {
    OLA_INFO << "ArtDmx too small, was " << packet_size <<
      " required at least " << header_size + 2;
    return;
  }

  if (packet.version[0] || packet.version[1] != ARTNET_VERSION) {
    OLA_INFO << "ArtDmx version mismatch, was " <<
      static_cast<int>(packet.version[0]) <<
      static_cast<int>(packet.version[1]) << " from " <<
      inet_ntoa(source_address);
    return;
  }

  uint16_t universe_id = LittleEndianToHost(packet.universe);
  uint16_t data_size = std::min(
      (unsigned int) (packet.length[0] << 8 + packet.length[1]),
      packet_size - header_size);

  for (unsigned int port_id = 0; port_id < ARTNET_MAX_PORTS; port_id++) {
    if (m_output_ports[port_id].enabled &&
        m_output_ports[port_id].universe_address == universe_id &&
        m_output_ports[port_id].on_data &&
        m_output_ports[port_id].buffer) {
      // update this port, doing a merge if necessary
      DMXSource source;
      source.address = source_address;
      source.timestamp = *m_wake_time;
      source.buffer.Set(packet.data, data_size);
      UpdatePortFromSource(&m_output_ports[port_id], source);
    }
  }
}


/*
 * Fill in the header for a packet
 */
void ArtNetNode::PopulatePacketHeader(artnet_packet *packet,
                                      uint16_t op_code) {
  strncpy(reinterpret_cast<char*>(packet->id), ARTNET_ID, sizeof(packet->id));
  packet->op_code = HostToLittleEndian(op_code);
}


/*
 * Send an ArtNet packet
 * @param packet
 * @param size the size of the packet, excluding the header portion
 * @param destination where to send the packet to
 */
bool ArtNetNode::SendPacket(const artnet_packet &packet,
                            unsigned int size,
                            struct sockaddr_in destination) {
  size += sizeof(packet.id) + sizeof(packet.op_code);
  ssize_t bytes_sent = m_socket->SendTo(
      reinterpret_cast<const uint8_t*>(&packet),
      size,
      destination);

  if (bytes_sent != size) {
    OLA_WARN << "Only sent " << bytes_sent << " of " << size;
    return false;
  }
  return true;
}


/*
 * Update a port from a source, merging if necessary
 */
void ArtNetNode::UpdatePortFromSource(OutputPort *port,
                                      const DMXSource &source) {
  TimeStamp merge_time_threshold = (
      *m_wake_time - TimeInterval(MERGE_TIMEOUT, 0));
  unsigned int first_empty_slot = MAX_MERGE_SOURCES;
  unsigned int source_slot = MAX_MERGE_SOURCES;
  unsigned int active_sources = 0;

  for (unsigned int i = 0; i < MAX_MERGE_SOURCES; i++) {
    if (port->sources[i].address.s_addr == source.address.s_addr)
      source_slot = i;
    // timeout old sources
    if (port->sources[i].timestamp < merge_time_threshold)
      port->sources[i].address.s_addr = 0;

    if (port->sources[i].address.s_addr != 0)
      active_sources++;
    else if (i < first_empty_slot)
      first_empty_slot = i;
  }

  if (source_slot == MAX_MERGE_SOURCES) {
    if (first_empty_slot == MAX_MERGE_SOURCES) {
      // No room at the inn
      OLA_WARN << "Max merge sources reached, ignoring";
      return;
    }
    if (active_sources == 0) {
      port->is_merging = false;
    } else {
      OLA_INFO << "Entered merge mode for universe " <<
        static_cast<int>(port->universe_address);
      port->is_merging = true;
      if (m_send_reply_on_change) {
        m_unsolicited_replies++;
        SendPollReply(m_destination);
      }
    }
    source_slot = first_empty_slot;
  } else if (active_sources == 1) {
    port->is_merging = false;
  }

  port->sources[source_slot] = source;

  // Now we need to merge
  if (port->merge_mode == ARTNET_MERGE_LTP) {
    // the current source is the latest
    (*port->buffer) = source.buffer;
  } else {
    // HTP merge
    bool first = true;
    for (unsigned int i = 0; i < MAX_MERGE_SOURCES; i++) {
      if (port->sources[i].address.s_addr != 0) {
        if (first) {
          (*port->buffer) = port->sources[i].buffer;
          first = false;
        } else {
          port->buffer->HTPMerge(port->sources[i].buffer);
        }
      }
    }
  }


  port->on_data->Run();
}


/*
 * Setup the networking components.
 */
bool ArtNetNode::InitNetwork() {
  m_socket = new UdpSocket();

  if (!m_socket->Init()) {
    OLA_WARN << "Socket init failed";
    delete m_socket;
    return false;
  }

  if (!m_socket->Bind(ARTNET_PORT)) {
    OLA_WARN << "Failed to bind to:" << ARTNET_PORT;
    delete m_socket;
    return false;
  }

  if (!m_socket->EnableBroadcast()) {
    OLA_WARN << "Failed to enable broadcasting";
    delete m_socket;
    return false;
  }

  m_socket->SetOnData(NewClosure(this, &ArtNetNode::SocketReady));
  return true;
}
}  // artnet
}  // plugin
}  // ola
