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

#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/artnet/ArtNetNode.h"


namespace ola {
namespace plugin {
namespace artnet {

using ola::Callback1;
using ola::Callback0;
using ola::network::AddressToString;
using ola::network::HostToLittleEndian;
using ola::network::HostToNetwork;
using ola::network::LittleEndianToHost;
using ola::network::NetworkToHost;
using ola::network::UdpSocket;
using std::pair;
using std::string;


const char ArtNetNodeImpl::ARTNET_ID[] = "Art-Net";

/*
 * Create a new node
 * @param interface the interface to use.
 * @param short_name the short node name
 * @param long_name the long node name
 * @param subnet_address the ArtNet 'subnet' address, 4 bits.
 */
ArtNetNodeImpl::ArtNetNodeImpl(const ola::network::Interface &interface,
                               ola::network::SelectServerInterface *ss,
                               bool always_broadcast,
                               ola::network::UdpSocketInterface *socket)
    : m_running(false),
      m_send_reply_on_change(true),
      m_short_name(""),
      m_long_name(""),
      m_broadcast_threshold(BROADCAST_THRESHOLD),
      m_unsolicited_replies(0),
      m_ss(ss),
      m_always_broadcast(always_broadcast),
      m_interface(interface),
      m_socket(socket),
      m_discovery_timeout(ola::network::INVALID_TIMEOUT) {

  // reset all the port structures
  for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++) {
    m_input_ports[i].universe_address = 0;
    m_input_ports[i].sequence_number = 0;
    m_input_ports[i].enabled = false;
    m_input_ports[i].on_tod = NULL;
    m_input_ports[i].discovery_running = false;
    m_input_ports[i].rdm_request_callback = NULL;
    m_input_ports[i].pending_request = NULL;
    m_input_ports[i].rdm_send_timeout = ola::network::INVALID_TIMEOUT;

    m_output_ports[i].universe_address = 0;
    m_output_ports[i].sequence_number = 0;
    m_output_ports[i].enabled = false;
    m_output_ports[i].is_merging = false;
    m_output_ports[i].merge_mode = ARTNET_MERGE_HTP;
    m_output_ports[i].buffer = NULL;
    m_output_ports[i].on_data = NULL;
    m_output_ports[i].on_discover = NULL;
    m_output_ports[i].on_flush = NULL;
    m_output_ports[i].on_rdm_request = NULL;
  for (unsigned int j = 0; j < MAX_MERGE_SOURCES; j++)
      m_output_ports[i].sources[j].address.s_addr = 0;
  }
}


/*
 * Cleanup
 */
ArtNetNodeImpl::~ArtNetNodeImpl() {
  Stop();

  for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++) {
    if (m_input_ports[i].on_tod)
      delete m_input_ports[i].on_tod;

    if (m_output_ports[i].on_data)
      delete m_output_ports[i].on_data;
    if (m_output_ports[i].on_discover)
      delete m_output_ports[i].on_discover;
    if (m_output_ports[i].on_flush)
      delete m_output_ports[i].on_flush;
    if (m_output_ports[i].on_rdm_request)
      delete m_output_ports[i].on_rdm_request;
  }
}


/*
 * Start this node. The port modifying functions can be called before this.
 */
bool ArtNetNodeImpl::Start() {
  if (m_running)
    return false;

  if (!InitNetwork())
    return false;

  m_running = true;

  SendPoll();  // send a poll, this will result in us repling as well
  SendPollReply(m_interface.bcast_address);
  return true;
}


/*
 * Stop this node
 */
bool ArtNetNodeImpl::Stop() {
  if (!m_running)
    return false;

  if (m_discovery_timeout != ola::network::INVALID_TIMEOUT) {
    m_ss->RemoveTimeout(m_discovery_timeout);
    m_discovery_timeout = ola::network::INVALID_TIMEOUT;
  }

  // clean up any in-flight rdm requests
  for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++) {
    InputPort &port = m_input_ports[i];
    if (port.rdm_send_timeout != ola::network::INVALID_TIMEOUT) {
      m_ss->RemoveTimeout(port.rdm_send_timeout);
      port.rdm_send_timeout = ola::network::INVALID_TIMEOUT;
    }

    if (port.rdm_request_callback)
      port.rdm_request_callback->Run(ola::rdm::RDM_TIMEOUT, NULL);
    if (port.pending_request)
      delete port.pending_request;
  }

  m_ss->RemoveSocket(m_socket);

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
bool ArtNetNodeImpl::SetShortName(const string &name) {
  m_short_name = name;
  if (m_running && m_send_reply_on_change) {
    m_unsolicited_replies++;
    return SendPollReply(m_interface.bcast_address);
  }
  return true;
}


/*
 * Set the long name
 */
bool ArtNetNodeImpl::SetLongName(const string &name) {
  m_long_name = name;
  if (m_running && m_send_reply_on_change) {
    m_unsolicited_replies++;
    return SendPollReply(m_interface.bcast_address);
  }
  return true;
}


/*
 * The the subnet address for this node
 */
bool ArtNetNodeImpl::SetSubnetAddress(uint8_t subnet_address) {
  uint8_t old_address = m_input_ports[0].universe_address >> 4;
  if (old_address == subnet_address)
    return true;

  subnet_address = subnet_address << 4;
  for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++) {
    m_input_ports[i].universe_address = subnet_address |
      (m_input_ports[i].universe_address & 0x0f);
    m_output_ports[i].universe_address = subnet_address |
      (m_output_ports[i].universe_address & 0x0f);
    m_input_ports[i].uids.clear();
  }

  if (m_running && m_send_reply_on_change) {
    m_unsolicited_replies++;
    return SendPollReply(m_interface.bcast_address);
  }
  return true;
}


/*
 * Set the universe for a port
 */
bool ArtNetNodeImpl::SetPortUniverse(artnet_port_type type,
                                 uint8_t port_id,
                                 uint8_t universe_id) {
  if (port_id > ARTNET_MAX_PORTS) {
    OLA_WARN << "Port index of out bounds: " << port_id << " > " <<
      ARTNET_MAX_PORTS;
    return false;
  }

  if (type == ARTNET_INPUT_PORT) {
    uint8_t old_universe = m_input_ports[port_id].universe_address;
    m_input_ports[port_id].universe_address = (
        (universe_id & 0x0f) |
        (m_input_ports[port_id].universe_address & 0xf0));

    if (old_universe != m_input_ports[port_id].universe_address)
      // clear the uid map
      m_input_ports[port_id].uids.clear();

    bool ports_previously_enabled = false;
    for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++)
      ports_previously_enabled |= m_input_ports[i].enabled;

    m_input_ports[port_id].enabled = universe_id != ARTNET_DISABLE_PORT;
    if (!ports_previously_enabled && m_input_ports[port_id].enabled)
      SendPoll();
  } else {
    m_output_ports[port_id].universe_address = (
        (universe_id & 0x0f) |
        (m_output_ports[port_id].universe_address & 0xf0));
    m_output_ports[port_id].enabled = universe_id != ARTNET_DISABLE_PORT;
  }

  if (m_running && m_send_reply_on_change) {
    m_unsolicited_replies++;
    return SendPollReply(m_interface.bcast_address);
  }
  return true;
}


/*
 * Return the current universe address for a port
 */
uint8_t ArtNetNodeImpl::GetPortUniverse(artnet_port_type type,
                                        uint8_t port_id) {
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
bool ArtNetNodeImpl::SetMergeMode(uint8_t port_id,
                                  artnet_merge_mode merge_mode) {
  if (port_id > ARTNET_MAX_PORTS) {
    OLA_WARN << "Port index of out bounds: " << port_id << " > " <<
      ARTNET_MAX_PORTS;
    return false;
  }

  m_output_ports[port_id].merge_mode = merge_mode;
  if (m_running && m_send_reply_on_change) {
    m_unsolicited_replies++;
    return SendPollReply(m_interface.bcast_address);
  }
  return true;
}


/*
 * Send an ArtPoll if any of the ports are sending data
 */
bool ArtNetNodeImpl::SendPoll() {
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
  packet.data.poll.version = HostToNetwork(ARTNET_VERSION);
  // send PollReplies when something changes
  packet.data.poll.talk_to_me = 0x02;
  unsigned int size = sizeof(packet.data.poll);

  return SendPacket(packet, size, m_interface.bcast_address);
}


/*
 * Send some DMX data
 * @param universe the id of the universe to send
 * @param buffer the DMX data
 * @return true if it was send successfully, false otherwise
 */
bool ArtNetNodeImpl::SendDMX(uint8_t port_id, const DmxBuffer &buffer) {
  if (!CheckInputPortState(port_id, "ArtDMX"))
    return false;

  if (!buffer.Size()) {
    OLA_DEBUG << "Not sending 0 length packet";
    return true;
  }

  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_DMX);
  memset(&packet.data.dmx, 0, sizeof(packet.data.dmx));

  packet.data.poll.version = HostToNetwork(ARTNET_VERSION);
  packet.data.dmx.sequence = m_input_ports[port_id].sequence_number;
  packet.data.dmx.physical = 1 + port_id;
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
  if (m_input_ports[port_id].subscribed_nodes.size() >= BROADCAST_THRESHOLD ||
      m_always_broadcast) {
    sent_ok = SendPacket(packet, size, m_interface.bcast_address);
    m_input_ports[port_id].sequence_number++;
  } else {
    map<struct in_addr, TimeStamp>::iterator iter =
      m_input_ports[port_id].subscribed_nodes.begin();

    TimeStamp last_heard_threshold = (
        *m_ss->WakeUpTime() - TimeInterval(NODE_TIMEOUT, 0));
    while (iter != m_input_ports[port_id].subscribed_nodes.end()) {
      // if this node has timed out, remove it from the set
      if (iter->second < last_heard_threshold) {
        m_input_ports[port_id].subscribed_nodes.erase(iter++);
        continue;
      }
      sent_ok |= SendPacket(packet, size, iter->first);
      ++iter;
    }

    if (!m_input_ports[port_id].subscribed_nodes.size()) {
      OLA_DEBUG <<
        "Suppressing data transmit due to no active nodes for universe " <<
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
 * Send a TODRequest
 * @param port_id port to send on
 */
bool ArtNetNodeImpl::SendTodRequest(uint8_t port_id) {
  if (!CheckInputPortState(port_id, "ArtTodRequest"))
    return false;

  if (!GrabDiscoveryLock(port_id))
    return true;

  OLA_DEBUG << "Sending ArtTodRequest for address " <<
    static_cast<int>(m_input_ports[port_id].universe_address);
  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_TODREQUEST);
  memset(&packet.data.tod_request, 0, sizeof(packet.data.tod_request));
  packet.data.tod_request.version = HostToNetwork(ARTNET_VERSION);
  packet.data.tod_request.address_count = 1;  // only one universe address
  packet.data.tod_request.addresses[0] =
    m_input_ports[port_id].universe_address;
  unsigned int size = sizeof(packet.data.tod_request);
  return SendPacket(packet, size, m_interface.bcast_address);
}


/*
 * Flush the TOD and force discovery
 * @param port_id port to send on
 */
bool ArtNetNodeImpl::ForceDiscovery(uint8_t port_id) {
  if (!CheckInputPortState(port_id, "ArtTodControl"))
    return false;

  if (!GrabDiscoveryLock(port_id))
    return true;

  OLA_DEBUG << "Sending ArtTodControl";
  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_TODCONTROL);
  memset(&packet.data.tod_control, 0, sizeof(packet.data.tod_control));
  packet.data.tod_control.version = HostToNetwork(ARTNET_VERSION);
  packet.data.tod_control.command = TOD_FLUSH_COMMAND;
  packet.data.tod_control.address = m_input_ports[port_id].universe_address;
  unsigned int size = sizeof(packet.data.tod_control);
  return SendPacket(packet, size, m_interface.bcast_address);
}


/*
 * Send an RDMRequest on this port, this may defer the sending if there are
 * other outstanding messages in the queue.
 * @param port_id the if of the port to send the request on
 * @param request the RDMRequest object
 *
 * Because this is wrapped in the QueueingRDMController this will only be
 * called one-at-a-time (per port)
 */
void ArtNetNodeImpl::SendRDMRequest(uint8_t port_id,
                                    const RDMRequest *request,
                                    RDMCallback *on_complete) {
  if (!CheckInputPortState(port_id, "ArtRDM")) {
    on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL);
    delete request;
    return;
  }

  InputPort &port = m_input_ports[port_id];
  if (port.rdm_request_callback) {
    OLA_FATAL << "Previous request hasn't completed yet, dropping request";
    on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL);
    delete request;
    return;
  }

  struct in_addr destination = m_interface.bcast_address;
  const UID &uid_destination = request->DestinationUID();
  uid_map::const_iterator iter = port.uids.find(uid_destination);
  if (iter == port.uids.end()) {
    if (!uid_destination.IsBroadcast())
      OLA_WARN << "Couldn't find " << uid_destination <<
        " in the uid map, broadcasting packet";
  } else {
    destination = iter->second.first;
  }

  port.rdm_request_callback = on_complete;
  port.pending_request = request;
  bool r = SendRDMCommand(*request,
                        destination,
                        port.universe_address);
  if (r) {
    if (uid_destination.IsBroadcast()) {
      port.rdm_request_callback = NULL;
      port.pending_request = NULL;
      delete request;
      on_complete->Run(ola::rdm::RDM_WAS_BROADCAST, NULL);
    } else {
      port.rdm_send_timeout = m_ss->RegisterSingleTimeout(
        RDM_REQUEST_TIMEOUT_MS,
        ola::NewSingleCallback(this,
                               &ArtNetNodeImpl::TimeoutRDMRequest,
                               port_id));
    }
  } else {
    port.rdm_request_callback = NULL;
    port.pending_request = NULL;
    delete request;
    on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL);
  }
}


/*
 * Set the RDM handlers for an Input port
 * @param port_id the id of the port to set the handlers for
 * @param on_tod the callback to be invoked when a ArtTod message is recieved,
 *   this callback is passed two UIDSets, one for the UIDs added and one for
 *   the UIDs that were removed
 */
bool ArtNetNodeImpl::SetInputPortRDMHandlers(
    uint8_t port_id,
    ola::Callback1<void, const ola::rdm::UIDSet&> *on_tod) {
  if (port_id > ARTNET_MAX_PORTS) {
    OLA_WARN << "Port index of out bounds: " << port_id << " > " <<
      ARTNET_MAX_PORTS;
    return false;
  }

  if (m_input_ports[port_id].on_tod)
    delete m_input_ports[port_id].on_tod;
  m_input_ports[port_id].on_tod = on_tod;
  return true;
}


/*
 * Set the closure to be called when we receive data for this universe.
 * @param universe the universe to register the handler for
 * @param handler the Callback0 to call when there is data for this universe.
 * Ownership of the closure is transferred to the node.
 */
bool ArtNetNodeImpl::SetDMXHandler(uint8_t port_id,
                               DmxBuffer *buffer,
                               Callback0<void> *on_data) {
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
 * Send an set of UIDs in one of more ArtTod packets
 */
bool ArtNetNodeImpl::SendTod(uint8_t port_id, const UIDSet &uid_set) {
  if (!CheckOutputPortState(port_id, "ArtTodData"))
    return false;

  OLA_DEBUG << "Sending ArtTodRequest";
  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_TODDATA);
  memset(&packet.data.tod_data, 0, sizeof(packet.data.tod_data));
  packet.data.tod_data.version = HostToNetwork(ARTNET_VERSION);
  packet.data.tod_data.rdm_version = RDM_VERSION;
  packet.data.tod_data.port = 1 + port_id;
  packet.data.tod_data.address = m_output_ports[port_id].universe_address;
  uint16_t uids = std::min(uid_set.Size(),
                           (unsigned int) MAX_UIDS_PER_UNIVERSE);
  packet.data.tod_data.uid_total = HostToNetwork(uids);
  packet.data.tod_data.uid_count = ARTNET_MAX_UID_COUNT;

  uint8_t (*ptr)[ola::rdm::UID::UID_SIZE] = packet.data.tod_data.tod;
  unsigned int i = 0;
  UIDSet::Iterator iter = uid_set.Begin();

  while (iter != uid_set.End()) {
    iter->Pack(*ptr, ola::rdm::UID::UID_SIZE);

    i++;
    iter++;
    if (i % ARTNET_MAX_UID_COUNT == 0) {
      packet.data.tod_data.block_count = i / ARTNET_MAX_UID_COUNT - 1;
      SendPacket(packet,
                 sizeof(packet.data.tod_data),
                 m_interface.bcast_address);
      ptr = packet.data.tod_data.tod;
    } else {
      ptr++;
    }
  }

  if (i == 0 || i % ARTNET_MAX_UID_COUNT) {
    packet.data.tod_data.uid_count = i % ARTNET_MAX_UID_COUNT;
    packet.data.tod_data.block_count = i / ARTNET_MAX_UID_COUNT;
    unsigned int size = sizeof(packet.data.tod_data) -
      sizeof(packet.data.tod_data.tod) + i * ola::rdm::UID::UID_SIZE;
    SendPacket(packet, size, m_interface.bcast_address);
  }
  return true;
}


/*
 * Set the RDM handlers for an Output port
 */
bool ArtNetNodeImpl::SetOutputPortRDMHandlers(
    uint8_t port_id,
    ola::Callback0<void> *on_discover,
    ola::Callback0<void> *on_flush,
    ola::Callback2<void, const RDMRequest*, RDMCallback*> *on_rdm_request) {

  if (port_id > ARTNET_MAX_PORTS) {
    OLA_WARN << "Port index of out bounds: " << port_id << " > " <<
      ARTNET_MAX_PORTS;
    return false;
  }

  if (m_output_ports[port_id].on_discover)
    delete m_output_ports[port_id].on_discover;
  if (m_output_ports[port_id].on_flush)
    delete m_output_ports[port_id].on_flush;
  if (m_output_ports[port_id].on_rdm_request)
    delete m_output_ports[port_id].on_rdm_request;
  m_output_ports[port_id].on_discover = on_discover;
  m_output_ports[port_id].on_flush = on_flush;
  m_output_ports[port_id].on_rdm_request = on_rdm_request;
  return true;
}


/*
 * Called when there is data on this socket
 */
void ArtNetNodeImpl::SocketReady() {
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
bool ArtNetNodeImpl::SendPollReply(const struct in_addr &destination) {
  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_REPLY);
  memset(&packet.data.reply, 0, sizeof(packet.data.reply));

  memcpy(packet.data.reply.ip,
         &m_interface.ip_address.s_addr,
         sizeof(packet.data.reply.ip));
  packet.data.reply.port = HostToLittleEndian(ARTNET_PORT);
  packet.data.reply.subnet_address[1] = m_input_ports[0].universe_address >> 4;
  packet.data.reply.oem = HostToNetwork(OEM_CODE);
  packet.data.reply.status1 = 0xd2;  // normal indicators, rdm enabled
  packet.data.reply.esta_id = HostToLittleEndian(OPEN_LIGHTING_ESTA_CODE);
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
 * Send an IPProgReply
 */
bool ArtNetNodeImpl::SendIPReply(const struct in_addr &destination) {
  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_REPLY);
  memset(&packet.data.ip_reply, 0, sizeof(packet.data.ip_reply));
  packet.data.ip_reply.version = HostToNetwork(ARTNET_VERSION);

  memcpy(packet.data.ip_reply.ip,
         &m_interface.ip_address.s_addr,
         sizeof(packet.data.ip_reply.ip));
  memcpy(packet.data.ip_reply.subnet,
         &m_interface.ip_address.s_addr,
         sizeof(packet.data.ip_reply.subnet));
  packet.data.ip_reply.port = HostToLittleEndian(ARTNET_PORT);

  if (!SendPacket(packet, sizeof(packet.data.ip_reply), destination)) {
    OLA_INFO << "Failed to send ArtIpProgReply";
    return false;
  }
  return true;
}


/*
 * Handle a artnet packet
 */
void ArtNetNodeImpl::HandlePacket(const struct in_addr &source_address,
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
    case ARTNET_TODREQUEST:
      HandleTodRequest(source_address,
                       packet.data.tod_request,
                       packet_size - header_size);
      break;
    case ARTNET_TODDATA:
      HandleTodData(source_address,
                    packet.data.tod_data,
                    packet_size - header_size);
      break;
    case ARTNET_TODCONTROL:
      HandleTodControl(source_address,
                       packet.data.tod_control,
                       packet_size - header_size);
      break;
    case ARTNET_RDM:
      HandleRdm(source_address,
                packet.data.rdm,
                packet_size - header_size);
      break;
    case ARTNET_IP_PROGRAM:
      HandleIPProgram(source_address,
                      packet.data.ip_program,
                      packet_size - header_size);
      break;
    case ARTNET_RDM_SUB:
      // Not implemented
      break;
    default:
      OLA_INFO << "ArtNet got unknown packet " << std::hex <<
        LittleEndianToHost(packet.op_code);
  }
}


/*
 * Handle an ArtPoll packet
 */
void ArtNetNodeImpl::HandlePollPacket(const struct in_addr &source_address,
                                  const artnet_poll_t &packet,
                                  unsigned int packet_size) {
  if (!CheckPacketSize(source_address, "ArtPoll", packet_size, sizeof(packet)))
    return;

  if (!CheckPacketVersion(source_address, "ArtPoll", packet.version))
    return;

  m_send_reply_on_change = packet.talk_to_me & 0x02;
  // It's unclear if this should be broadcast or unicast, stick with broadcast
  SendPollReply(m_interface.bcast_address);
  (void) source_address;
}


/*
 * Handle an ArtPollReply packet
 */
void ArtNetNodeImpl::HandleReplyPacket(const struct in_addr &source_address,
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
  if (!CheckPacketSize(source_address, "ArtPollReply", packet_size,
        minimum_reply_size))
    return;

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
            *m_ss->WakeUpTime();
        }
      }
    }
  }
}


/*
 * Handle a DMX Data packet, this takes care of the merging
 */
void ArtNetNodeImpl::HandleDataPacket(const struct in_addr &source_address,
                                  const artnet_dmx_t &packet,
                                  unsigned int packet_size) {
  // The data section needs to be at least 2 bytes according to the spec
  unsigned int header_size = sizeof(artnet_dmx_t) - DMX_UNIVERSE_SIZE;
  if (!CheckPacketSize(source_address, "ArtDmx", packet_size, header_size + 2))
    return;

  if (!CheckPacketVersion(source_address, "ArtDmx", packet.version))
    return;

  uint16_t universe_id = LittleEndianToHost(packet.universe);
  uint16_t data_size = std::min(
      (unsigned int) ((packet.length[0] << 8) + packet.length[1]),
      packet_size - header_size);

  for (unsigned int port_id = 0; port_id < ARTNET_MAX_PORTS; port_id++) {
    if (m_output_ports[port_id].enabled &&
        m_output_ports[port_id].universe_address == universe_id &&
        m_output_ports[port_id].on_data &&
        m_output_ports[port_id].buffer) {
      // update this port, doing a merge if necessary
      DMXSource source;
      source.address = source_address;
      source.timestamp = *m_ss->WakeUpTime();
      source.buffer.Set(packet.data, data_size);
      UpdatePortFromSource(&m_output_ports[port_id], source);
    }
  }
}


/*
 * Handle a TOD Request packet
 */
void ArtNetNodeImpl::HandleTodRequest(const struct in_addr &source_address,
                                      const artnet_todrequest_t &packet,
                                      unsigned int packet_size) {
  unsigned int header_size = sizeof(packet) - sizeof(packet.addresses);
  if (!CheckPacketSize(source_address, "ArtTodRequest", packet_size,
                       header_size))
    return;

  if (!CheckPacketVersion(source_address, "ArtTodRequest", packet.version))
    return;

  if (packet.command) {
    OLA_INFO << "ArtTodRequest received but command field was " <<
      static_cast<int>(packet.command);
    return;
  }

  unsigned int addresses = std::min(
      packet_size - header_size,
      static_cast<unsigned int>(packet.address_count));

  addresses = std::min(
      static_cast<unsigned int>(ARTNET_MAX_RDM_ADDRESS_COUNT),
      addresses);

  bool handler_called[ARTNET_MAX_PORTS];
  memset(handler_called, 0, sizeof(handler_called));

  for (unsigned int i = 0; i < addresses; i++) {
    for (unsigned int port_id = 0; port_id < ARTNET_MAX_PORTS; port_id++) {
      if (m_output_ports[port_id].enabled &&
          m_output_ports[port_id].universe_address == packet.addresses[i] &&
          m_output_ports[port_id].on_discover &&
          !handler_called[port_id]) {
        m_output_ports[port_id].on_discover->Run();
        handler_called[port_id] = true;
      }
    }
  }
}


/*
 * Handle a TOD data packet
 */
void ArtNetNodeImpl::HandleTodData(const struct in_addr &source_address,
                               const artnet_toddata_t &packet,
                               unsigned int packet_size) {
  unsigned int expected_size = sizeof(packet) - sizeof(packet.tod);
  if (!CheckPacketSize(source_address, "ArtTodData", packet_size,
                       expected_size))
    return;

  if (!CheckPacketVersion(source_address, "ArtTodData", packet.version))
    return;

  if (packet.rdm_version != RDM_VERSION) {
    OLA_WARN << "Dropping non standard RDM version: " <<
      static_cast<int>(packet.rdm_version);
    return;
  }

  if (packet.command_response) {
    OLA_WARN << "Command response 0x" << std::hex << packet.command_response <<
      " != 0x0";
    return;
  }

  for (unsigned int port_id = 0; port_id < ARTNET_MAX_PORTS; port_id++) {
    if (m_input_ports[port_id].enabled &&
        m_input_ports[port_id].universe_address == packet.address) {
      UpdatePortFromTodPacket(port_id, source_address, packet, packet_size);
    }
  }
}


/*
 * Handle a TOD Control packet
 */
void ArtNetNodeImpl::HandleTodControl(const struct in_addr &source_address,
                                  const artnet_todcontrol_t &packet,
                                  unsigned int packet_size) {
  if (!CheckPacketSize(source_address, "ArtTodControl", packet_size,
                       sizeof(packet)))
    return;

  if (!CheckPacketVersion(source_address, "ArtTodControl", packet.version))
    return;

  if (packet.command != TOD_FLUSH_COMMAND)
    return;

  for (unsigned int port_id = 0; port_id < ARTNET_MAX_PORTS; port_id++) {
    if (m_output_ports[port_id].enabled &&
        m_output_ports[port_id].universe_address == packet.address &&
        m_output_ports[port_id].on_flush) {
      m_output_ports[port_id].on_flush->Run();
    }
  }
}


/*
 * Handle an RDM packet
 */
void ArtNetNodeImpl::HandleRdm(const struct in_addr &source_address,
                               const artnet_rdm_t &packet,
                               unsigned int packet_size) {
  unsigned int header_size = sizeof(packet) - ARTNET_MAX_RDM_DATA;
  if (!CheckPacketSize(source_address, "ArtRDM", packet_size, header_size))
    return;

  if (!CheckPacketVersion(source_address, "ArtRDM", packet.version))
    return;

  if (packet.rdm_version != RDM_VERSION) {
    OLA_INFO << "Dropping non standard RDM version: " <<
      static_cast<int>(packet.rdm_version);
    return;
  }

  if (packet.command) {
    OLA_WARN << "Unknown RDM command " << static_cast<int>(packet.command);
    return;
  }

  unsigned int rdm_length = packet_size - header_size;
  if (!rdm_length)
    return;

  // look for the port that this was sent to, once we know the port we can try
  // to parse the message
  for (uint8_t port_id = 0; port_id < ARTNET_MAX_PORTS; port_id++) {
    if (m_output_ports[port_id].enabled &&
        m_output_ports[port_id].universe_address == packet.address &&
        m_output_ports[port_id].on_rdm_request) {
      RDMRequest *request = RDMRequest::InflateFromData(packet.data,
                                                        rdm_length);

      if (request) {
        m_output_ports[port_id].on_rdm_request->Run(
            request,
            NewCallback(this,
                        &ArtNetNodeImpl::RDMRequestCompletion,
                        source_address,
                        port_id,
                        m_output_ports[port_id].universe_address));
      }
    }

    if (m_input_ports[port_id].enabled &&
        m_input_ports[port_id].universe_address == packet.address) {
      RDMResponse *response = RDMResponse::InflateFromData(packet.data,
                                                           rdm_length);
      if (response)
        HandleRDMResponse(port_id, response);
    }
  }
}


/**
 * Handle the completion of a request for an Output port
 */
void ArtNetNodeImpl::RDMRequestCompletion(struct in_addr destination,
                                          uint8_t port_id,
                                          uint8_t universe_address,
                                          ola::rdm::rdm_request_status status,
                                          const RDMResponse *response) {
  if (!CheckOutputPortState(port_id, "ArtRDM")) {
    if (response)
      delete response;
    return;
  }

  if (m_output_ports[port_id].universe_address == universe_address) {
    if (status == ola::rdm::RDM_COMPLETED_OK) {
      // TODO(simon): handle fragmenation here
      SendRDMCommand(*response,
                     destination,
                     universe_address);
    } else if (status == ola::rdm::RDM_UNKNOWN_UID) {
      // call the on discovery handler, which will send a new TOD and
      // hopefully update the remote controller
      m_output_ports[port_id].on_discover->Run();
    } else {
      OLA_WARN << "ArtNet RDM request failed with code " << status;
    }
  } else {
    // the universe address has changed we need to drop this request
    OLA_WARN <<
      "ArtNet Output port has changed mid request, dropping response";
  }
  if (response)
    delete response;
}


/**
 * Handle an RDM response, taking care to deal with ACK_OVERFLOW messages.
 * <rant>
 * ArtNet as a protocol is broken, the nodes don't buffer ACK_OVERFLOW messages
 * so if another GET/SET message arrives from *any* controller the ACK_OVERFLOW
 * session will be reset, possibly causing the controller to spin in a loop.
 * </rant>
 */
void ArtNetNodeImpl::HandleRDMResponse(unsigned int port_id,
                                       const RDMResponse *response) {
  InputPort &input_port = m_input_ports[port_id];
  if (!input_port.pending_request) {
    delete response;
    return;
  }

  const RDMRequest *request = input_port.pending_request;
  if (request->SourceUID() != response->DestinationUID() ||
      request->DestinationUID() != response->SourceUID() ||
      request->SubDevice() != response->SubDevice() ||
      request->ParamId() != response->ParamId()) {
    OLA_INFO << "Got an unexpected RDM response";
    delete response;
    return;
  }

  if ((request->CommandClass() == RDMCommand::GET_COMMAND &&
       response->CommandClass() != RDMCommand::GET_COMMAND_RESPONSE) ||
      (request->CommandClass() == RDMCommand::SET_COMMAND &&
       response->CommandClass() != RDMCommand::SET_COMMAND_RESPONSE)) {
    OLA_INFO << "Unmatched RDM response, request CC was " << std::hex <<
      static_cast<int>(request->CommandClass()) << ", response CC was " <<
      static_cast<int>(response->CommandClass());
    return;
  }

  // at this point we've decided it's for us
  // TODO(simon): check the src address as well?
  input_port.pending_request = NULL;
  ola::rdm::RDMCallback *callback = input_port.rdm_request_callback;
  input_port.rdm_request_callback = NULL;

  // remove the timeout
  if (input_port.rdm_send_timeout != ola::network::INVALID_TIMEOUT) {
    m_ss->RemoveTimeout(input_port.rdm_send_timeout);
    input_port.rdm_send_timeout = ola::network::INVALID_TIMEOUT;
  }

  callback->Run(ola::rdm::RDM_COMPLETED_OK, response);
}


/**
 * Handle an IP Program message.
 */
void ArtNetNodeImpl::HandleIPProgram(const struct in_addr &source_address,
                                     const artnet_ip_prog_t &packet,
                                     unsigned int packet_size) {
  if (!CheckPacketSize(source_address, "ArtIpProg", packet_size,
                       sizeof(packet)))
    return;

  if (!CheckPacketVersion(source_address, "ArtIpProg", packet.version))
    return;

  OLA_INFO <<
    "Got ArtIpProgram, ignoring because we don't support remote configuration";
}

/*
 * Fill in the header for a packet
 */
void ArtNetNodeImpl::PopulatePacketHeader(artnet_packet *packet,
                                      uint16_t op_code) {
  strncpy(reinterpret_cast<char*>(packet->id), ARTNET_ID, sizeof(packet->id));
  packet->op_code = HostToLittleEndian(op_code);
}



/*
 * Increment the counts for all the uids
 */
void ArtNetNodeImpl::IncrementUIDCounts(uint8_t port_id) {
  uid_map::iterator iter = m_input_ports[port_id].uids.begin();

  for (; iter != m_input_ports[port_id].uids.end(); ++iter) {
    iter->second.second++;
  }
}


/*
 * Send an ArtNet packet
 * @param packet
 * @param size the size of the packet, excluding the header portion
 * @param destination where to send the packet to
 */
bool ArtNetNodeImpl::SendPacket(const artnet_packet &packet,
                            unsigned int size,
                            const struct in_addr &ip_destination) {
  struct sockaddr_in destination;
  destination.sin_family = AF_INET;
  destination.sin_port = HostToNetwork(ARTNET_PORT);
  destination.sin_addr = ip_destination;

  size += sizeof(packet.id) + sizeof(packet.op_code);
  unsigned int bytes_sent = m_socket->SendTo(
      reinterpret_cast<const uint8_t*>(&packet),
      size,
      destination);

  if (bytes_sent != size) {
    OLA_WARN << "Only sent " << bytes_sent << " of " << size;
    return false;
  }
  return true;
}


/**
 * Timeout a pending RDM request
 * @param port_id the id of the port to timeout.
 */
void ArtNetNodeImpl::TimeoutRDMRequest(uint8_t port_id) {
  OLA_INFO << "RDM Request timed out.";
  m_input_ports[port_id].rdm_send_timeout = ola::network::INVALID_TIMEOUT;
  InputPort &port = m_input_ports[port_id];
  delete port.pending_request;
  ola::rdm::RDMCallback *callback = port.rdm_request_callback;
  port.rdm_request_callback = NULL;
  callback->Run(ola::rdm::RDM_TIMEOUT, NULL);
}


/*
 * Send a generic ArtRdm message
 */
bool ArtNetNodeImpl::SendRDMCommand(const RDMCommand &command,
                                    const struct in_addr &destination,
                                    uint8_t universe) {
  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_RDM);
  memset(&packet.data.rdm, 0, sizeof(packet.data.rdm));
  packet.data.rdm.version = HostToNetwork(ARTNET_VERSION);
  packet.data.rdm.rdm_version = RDM_VERSION;
  packet.data.rdm.address = universe;
  unsigned int rdm_size = ARTNET_MAX_RDM_DATA;
  command.Pack(packet.data.rdm.data, &rdm_size);
  unsigned int packet_size = sizeof(packet.data.rdm) - ARTNET_MAX_RDM_DATA +
    rdm_size;
  return SendPacket(packet, packet_size, destination);
}


/*
 * Update a port from a source, merging if necessary
 */
void ArtNetNodeImpl::UpdatePortFromSource(OutputPort *port,
                                          const DMXSource &source) {
  TimeStamp merge_time_threshold = (
      *m_ss->WakeUpTime() - TimeInterval(MERGE_TIMEOUT, 0));
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
        SendPollReply(m_interface.bcast_address);
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
 * Check the version number of a incomming packet
 */
bool ArtNetNodeImpl::CheckPacketVersion(const struct in_addr &source_address,
                                    const string &packet_type,
                                    uint16_t version) {
  if (NetworkToHost(version) != ARTNET_VERSION) {
    OLA_INFO << packet_type << " version mismatch, was " <<
      NetworkToHost(version) << " from " <<
      AddressToString(source_address);
    return false;
  }
  return true;
}


/*
 * Check the size of an incoming packet
 */
bool ArtNetNodeImpl::CheckPacketSize(const struct in_addr &source_address,
                                 const string &packet_type,
                                 unsigned int actual_size,
                                 unsigned int expected_size) {
  if (actual_size < expected_size) {
    OLA_INFO << packet_type << " from " << AddressToString(source_address) <<
      " was too small, got " << actual_size <<
      " required at least " << expected_size;
    return false;
  }
  return true;
}


/*
 * Check if the input port is available for sending
 */
bool ArtNetNodeImpl::CheckInputPortState(uint8_t port_id,
                                         const string &action) {
  return CheckPortState(port_id, action, false);
}


/*
 * Check if the output port is available for sending
 */
bool ArtNetNodeImpl::CheckOutputPortState(uint8_t port_id,
                                          const string &action) {
  return CheckPortState(port_id, action, true);
}


/*
 * Check if a port is available for sending
 */
bool ArtNetNodeImpl::CheckPortState(uint8_t port_id,
                                const string &action,
                                bool is_output) {
  if (!m_running)
    return false;

  if (port_id > ARTNET_MAX_PORTS) {
    OLA_WARN << "Port index of out bounds: " << port_id << " > " <<
      ARTNET_MAX_PORTS;
    return false;
  }

  if ((is_output && !m_output_ports[port_id].enabled) ||
      (!is_output && !m_input_ports[port_id].enabled)) {
    OLA_INFO << "Attempt to send " << action << " on an inactive port";
    return false;
  }
  return true;
}


/*
 * Setup the networking components.
 */
bool ArtNetNodeImpl::InitNetwork() {
  if (!m_socket)
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

  m_socket->SetOnData(NewCallback(this, &ArtNetNodeImpl::SocketReady));
  m_ss->AddSocket(m_socket);
  return true;
}


/*
 * Update a port with a new TOD list
 */
void ArtNetNodeImpl::UpdatePortFromTodPacket(uint8_t port_id,
                                         const struct in_addr &source_address,
                                         const artnet_toddata_t &packet,
                                         unsigned int packet_size) {
  unsigned int tod_size = packet_size - (sizeof(packet) - sizeof(packet.tod));
  unsigned int uid_count = std::min(tod_size / ola::rdm::UID::UID_SIZE,
                                    (unsigned int) packet.uid_count);

  OLA_DEBUG << "Got TOD data packet with " << uid_count << " uids";
  bool changed = false;
  uid_map &port_uids = m_input_ports[port_id].uids;
  UIDSet uid_set;

  for (unsigned int i = 0; i < uid_count; i++) {
    UID uid(packet.tod[i]);
    uid_set.AddUID(uid);
    uid_map::iterator iter = port_uids.find(uid);
    if (iter == port_uids.end()) {
      port_uids[uid] = std::pair<struct in_addr, uint8_t>(source_address, 0);
      changed = true;
    } else {
      if (iter->second.first.s_addr != source_address.s_addr) {
        OLA_WARN << "UID " << uid << " changed from " <<
          AddressToString(iter->second.first) << " to " <<
          AddressToString(source_address);
        iter->second.first = source_address;
      }
      iter->second.second = 0;
    }
  }

  // If this is the one and only block from this node, we can remove all uids
  // that don't appear in it.
  // There is a bug in ArtNet nodes where sometimes UidCount > UidTotal.
  if (uid_count >= NetworkToHost(packet.uid_total)) {
    uid_map::iterator iter = port_uids.begin();
    while (iter != port_uids.end()) {
      if (iter->second.first.s_addr == source_address.s_addr &&
          !uid_set.Contains(iter->first)) {
        port_uids.erase(iter++);
        changed = true;
      } else {
        ++iter;
      }
    }
  }

  // Removing uids from multi-block messages is much harder as you need to
  // consider dropped packets. For the moment we rely on the
  // RDM_MISSED_TODDATA_LIMIT to clean these up.
  // TODO(simon): figure this out sometime

  if (changed)
    NotifyClientOfNewTod(port_id);
}


/*
 * Start the discovery process, this puts the port into discovery mode and
 * sets up the completion callback
 */
bool ArtNetNodeImpl::GrabDiscoveryLock(uint8_t port_id) {
  if (m_input_ports[port_id].discovery_running) {
    OLA_INFO <<
      "ArtNet UID discovery already running, ignoring additional requests";
    return false;
  }
  m_input_ports[port_id].discovery_running = true;

  // increment the count of all current uids
  uid_map::iterator iter = m_input_ports[port_id].uids.begin();
  for (; iter != m_input_ports[port_id].uids.end(); ++iter) {
    iter->second.second++;
  }

  m_discovery_timeout = m_ss->RegisterSingleTimeout(
      RDM_TOD_TIMEOUT_MS,
      ola::NewSingleCallback(this,
                             &ArtNetNodeImpl::ReleaseDiscoveryLock,
                             port_id));
  return true;
}


/*
 * Called when the discovery process times out.
 */
void ArtNetNodeImpl::ReleaseDiscoveryLock(uint8_t port_id) {
  OLA_INFO << "Discovery process timeout";
  m_discovery_timeout = ola::network::INVALID_TIMEOUT;

  // delete all uids that have reached the max count
  bool changed = false;
  uid_map::iterator iter = m_input_ports[port_id].uids.begin();
  while (iter != m_input_ports[port_id].uids.end()) {
    if (iter->second.second == RDM_MISSED_TODDATA_LIMIT) {
      changed = true;
      m_input_ports[port_id].uids.erase(iter++);
    } else {
      ++iter;
    }
  }

  if (changed) {
    OLA_INFO << "Some uids have timed out, updating.";
    NotifyClientOfNewTod(port_id);
  }
  m_input_ports[port_id].discovery_running = false;
}


/*
 * Notify the client of a new TOD
 */
void ArtNetNodeImpl::NotifyClientOfNewTod(uint8_t port_id) {
  if (m_input_ports[port_id].on_tod) {
    uid_map &port_uids = m_input_ports[port_id].uids;
    UIDSet uids;
    uid_map::iterator uid_iter = port_uids.begin();
    for (; uid_iter != port_uids.end(); ++uid_iter)
      uids.AddUID(uid_iter->first);
    m_input_ports[port_id].on_tod->Run(uids);
  }
}



/**
 * DmxTriWidget Constructor
 */
ArtNetNode::ArtNetNode(const ola::network::Interface &interface,
                       ola::network::SelectServerInterface *ss,
                       bool always_broadcast,
                       unsigned int rdm_queue_size,
                       ola::network::UdpSocketInterface *socket):
    m_impl(interface, ss, always_broadcast, socket) {
  for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++) {
    m_wrappers[i] = new ArtNetNodeImplRDMWrapper(&m_impl, i);
    m_controllers[i] = new ola::rdm::QueueingRDMController(
        m_wrappers[i],
        rdm_queue_size);
  }
}


ArtNetNode::~ArtNetNode() {
  for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++) {
    delete m_controllers[i];
    delete m_wrappers[i];
  }
}


/**
 * Send a RDM request by passing it though the Queuing Controller
 */
void ArtNetNode::SendRDMRequest(uint8_t port_id, const RDMRequest *request,
                                ola::rdm::RDMCallback *on_complete) {
  if (port_id > ARTNET_MAX_PORTS) {
    OLA_WARN << "Port index of out bounds: " << port_id << " > " <<
      ARTNET_MAX_PORTS;
    on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL);
    delete request;
  }
  m_wrappers[port_id]->SendRDMRequest(request, on_complete);
}
}  // artnet
}  // plugin
}  // ola
