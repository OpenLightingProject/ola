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
 * ArtNetNode.cpp
 * An Art-Net node
 * Copyright (C) 2005 Simon Newton
 */

#include <string.h>
#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SocketAddress.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/stl/STLUtils.h"
#include "ola/strings/Format.h"
#include "ola/strings/Utils.h"
#include "plugins/artnet/ArtNetNode.h"


namespace ola {
namespace plugin {
namespace artnet {

using ola::Callback0;
using ola::Callback1;
using ola::network::HostToLittleEndian;
using ola::network::HostToNetwork;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::LittleEndianToHost;
using ola::network::NetworkToHost;
using ola::network::UDPSocket;
using ola::rdm::RDMCallback;
using ola::rdm::RDMCommand;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMDiscoveryCallback;
using ola::rdm::RDMFrame;
using ola::rdm::RDMReply;
using ola::rdm::RDMRequest;
using ola::rdm::RunRDMCallback;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using ola::strings::CopyToFixedLengthBuffer;
using ola::strings::ToHex;
using std::auto_ptr;
using std::map;
using std::pair;
using std::set;
using std::string;
using std::vector;


const char ArtNetNodeImpl::ARTNET_ID[] = "Art-Net";


// UID to the IP Address it came from, and the number of times since we last
// saw it in an ArtTod message.
typedef map<UID, std::pair<IPV4Address, uint8_t> > uid_map;

// Input ports are ones that send data using Art-Net
class ArtNetNodeImpl::InputPort {
 public:
  InputPort()
      : enabled(false),
        sequence_number(0),
        discovery_callback(NULL),
        discovery_timeout(ola::thread::INVALID_TIMEOUT),
        rdm_request_callback(NULL),
        pending_request(NULL),
        rdm_send_timeout(ola::thread::INVALID_TIMEOUT),
        m_port_address(0),
        m_tod_callback(NULL) {
  }
  ~InputPort() {}

  // Returns true if the address changed.
  bool SetUniverseAddress(uint8_t universe_address) {
    universe_address = universe_address & 0x0f;
    if ((m_port_address & 0x0f) == universe_address) {
      return false;
    }

    m_port_address = ((m_port_address & 0xf0) | universe_address);
    uids.clear();
    subscribed_nodes.clear();
    return true;
  }

  void ClearSubscribedNodes() {
    subscribed_nodes.clear();
  }

  // Returns true if the address changed.
  bool SetSubNetAddress(uint8_t subnet_address) {
    subnet_address = subnet_address << 4;
    if (subnet_address == (m_port_address & 0xf0)) {
      return false;
    }

    m_port_address = subnet_address | (m_port_address & 0x0f);
    uids.clear();
    subscribed_nodes.clear();
    return true;
  }

  // The 8-bit port address, which is made up of the sub-net and universe
  // address.
  uint8_t PortAddress() const {
    return m_port_address;
  }

  void SetTodCallback(RDMDiscoveryCallback *callback) {
    m_tod_callback.reset(callback);
  }

  void RunTodCallback() {
    if (m_tod_callback.get()) {
      RunRDMCallbackWithUIDs(uids, m_tod_callback.get());
    }
  }

  void RunDiscoveryCallback() {
    if (discovery_callback) {
      RDMDiscoveryCallback *callback = discovery_callback;
      discovery_callback = NULL;
      RunRDMCallbackWithUIDs(uids, callback);
    }
  }

  void IncrementUIDCounts() {
    for (uid_map::iterator iter = uids.begin(); iter != uids.end(); ++iter) {
      iter->second.second++;
    }
  }

  bool enabled;
  uint8_t sequence_number;
  map<IPV4Address, TimeStamp> subscribed_nodes;
  uid_map uids;  // used to keep track of the UIDs
  // NULL if discovery isn't running, otherwise the callback to run when it
  // finishes
  RDMDiscoveryCallback *discovery_callback;
  // The set of nodes we're expecting a response from
  set<IPV4Address> discovery_node_set;
  // the timeout_id for the discovery timer
  ola::thread::timeout_id discovery_timeout;
  // the in-flight request and it's callback
  RDMCallback *rdm_request_callback;
  const RDMRequest *pending_request;
  IPV4Address rdm_ip_destination;

  // these control the sending of RDM requests.
  ola::thread::timeout_id rdm_send_timeout;

 private:
  uint8_t m_port_address;
  // The callback to run if we receive an TOD and the discovery process
  // isn't running
  auto_ptr<RDMDiscoveryCallback> m_tod_callback;

  void RunRDMCallbackWithUIDs(const uid_map &uids,
                              RDMDiscoveryCallback *callback) {
    UIDSet uid_set;
    uid_map::const_iterator uid_iter = uids.begin();
    for (; uid_iter != uids.end(); ++uid_iter) {
      uid_set.AddUID(uid_iter->first);
    }
    callback->Run(uid_set);
  }
};

ArtNetNodeImpl::ArtNetNodeImpl(const ola::network::Interface &iface,
                               ola::io::SelectServerInterface *ss,
                               const ArtNetNodeOptions &options,
                               ola::network::UDPSocketInterface *socket)
    : m_running(false),
      m_net_address(0),
      m_send_reply_on_change(true),
      m_short_name(""),
      m_long_name(""),
      m_broadcast_threshold(options.broadcast_threshold),
      m_unsolicited_replies(0),
      m_ss(ss),
      m_always_broadcast(options.always_broadcast),
      m_use_limited_broadcast_address(options.use_limited_broadcast_address),
      m_in_configuration_mode(false),
      m_artpoll_required(false),
      m_artpollreply_required(false),
      m_interface(iface),
      m_socket(socket) {

  if (!m_socket.get()) {
    m_socket.reset(new UDPSocket());
  }

  for (unsigned int i = 0; i < options.input_port_count; i++) {
    m_input_ports.push_back(new InputPort());
  }

  // reset all the port structures
  for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++) {
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
  }
}

ArtNetNodeImpl::~ArtNetNodeImpl() {
  Stop();

  STLDeleteElements(&m_input_ports);

  for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++) {
    if (m_output_ports[i].on_data) {
      delete m_output_ports[i].on_data;
    }
    if (m_output_ports[i].on_discover) {
      delete m_output_ports[i].on_discover;
    }
    if (m_output_ports[i].on_flush) {
      delete m_output_ports[i].on_flush;
    }
    if (m_output_ports[i].on_rdm_request) {
      delete m_output_ports[i].on_rdm_request;
    }
  }
}

bool ArtNetNodeImpl::Start() {
  if (m_running || !InitNetwork()) {
    return false;
  }

  m_running = true;
  return true;
}

bool ArtNetNodeImpl::Stop() {
  if (!m_running) {
    return false;
  }

  // clean up any in-flight rdm requests
  InputPorts::iterator iter = m_input_ports.begin();
  for (; iter != m_input_ports.end(); ++iter) {
    InputPort *port = *iter;

    // clean up discovery state
    if (port->discovery_timeout != ola::thread::INVALID_TIMEOUT) {
      m_ss->RemoveTimeout(port->discovery_timeout);
      port->discovery_timeout = ola::thread::INVALID_TIMEOUT;
    }

    port->RunDiscoveryCallback();

    // clean up request state
    if (port->rdm_send_timeout != ola::thread::INVALID_TIMEOUT) {
      m_ss->RemoveTimeout(port->rdm_send_timeout);
      port->rdm_send_timeout = ola::thread::INVALID_TIMEOUT;
    }

    if (port->pending_request) {
      delete port->pending_request;
      port->pending_request = NULL;
    }

    if (port->rdm_request_callback) {
      RDMCallback *callback = port->rdm_request_callback;
      port->rdm_request_callback = NULL;
      RunRDMCallback(callback, ola::rdm::RDM_TIMEOUT);
    }
  }

  m_ss->RemoveReadDescriptor(m_socket.get());

  m_running = false;
  return true;
}

bool ArtNetNodeImpl::EnterConfigurationMode() {
  if (m_in_configuration_mode) {
    return false;
  }
  m_in_configuration_mode = true;
  m_artpoll_required = false;
  m_artpollreply_required = false;
  return true;
}

bool ArtNetNodeImpl::ExitConfigurationMode() {
  if (!m_in_configuration_mode) {
    return false;
  }
  m_in_configuration_mode = false;

  if (m_artpoll_required) {
    SendPoll();
    m_artpoll_required = false;
  }

  if (m_artpollreply_required) {
    SendPollReplyIfRequired();
  }
  return true;
}

bool ArtNetNodeImpl::SetShortName(const string &name) {
  if (m_short_name == name) {
    return true;
  }

  m_short_name = name;
  return SendPollReplyIfRequired();
}

bool ArtNetNodeImpl::SetLongName(const string &name) {
  if (m_long_name == name) {
    return true;
  }
  m_long_name = name;
  return SendPollReplyIfRequired();
}

bool ArtNetNodeImpl::SetNetAddress(uint8_t net_address) {
  if (net_address & 0x80) {
    OLA_WARN << "Art-Net net address > 127, truncating";
    net_address = net_address & 0x7f;
  }
  if (net_address == m_net_address) {
    return true;
  }

  m_net_address = net_address;

  bool input_ports_enabled = false;
  vector<InputPort*>::iterator iter = m_input_ports.begin();
  for (; iter != m_input_ports.end(); ++iter) {
    input_ports_enabled |= (*iter)->enabled;
    (*iter)->ClearSubscribedNodes();
  }

  if (input_ports_enabled) {
    SendPollIfAllowed();
  }
  return SendPollReplyIfRequired();
}

bool ArtNetNodeImpl::SetSubnetAddress(uint8_t subnet_address) {
  // Set for all input ports.
  bool changed = false;
  bool input_ports_enabled = false;
  vector<InputPort*>::iterator iter = m_input_ports.begin();
  for (; iter != m_input_ports.end(); ++iter) {
    input_ports_enabled |= (*iter)->enabled;
    changed |= (*iter)->SetSubNetAddress(subnet_address);
  }

  if (input_ports_enabled && changed) {
    SendPollIfAllowed();
  }

  // set for all output ports.
  uint8_t old_address = m_output_ports[0].universe_address >> 4;
  if (old_address == subnet_address && !changed) {
    return true;
  }

  subnet_address = subnet_address << 4;
  for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++) {
    m_output_ports[i].universe_address = subnet_address |
        (m_output_ports[i].universe_address & 0x0f);
  }

  return SendPollReplyIfRequired();
}

uint8_t ArtNetNodeImpl::InputPortCount() const {
  return m_input_ports.size();
}

bool ArtNetNodeImpl::SetInputPortUniverse(uint8_t port_id,
                                          uint8_t universe_id) {
  InputPort *port = GetInputPort(port_id);
  if (!port) {
    return false;
  }

  port->enabled = true;
  if (port->SetUniverseAddress(universe_id)) {
    SendPollIfAllowed();
    return SendPollReplyIfRequired();
  }
  return true;
}

uint8_t ArtNetNodeImpl::GetInputPortUniverse(uint8_t port_id) const {
  const InputPort *port = GetInputPort(port_id);
  return port ? port->PortAddress() : 0;
}

void ArtNetNodeImpl::DisableInputPort(uint8_t port_id) {
  InputPort *port = GetInputPort(port_id);
  bool was_enabled = false;
  if (port) {
    was_enabled = port->enabled;
    port->enabled = false;
  }

  if (was_enabled) {
    SendPollReplyIfRequired();
  }
}

bool ArtNetNodeImpl::InputPortState(uint8_t port_id) const {
  const InputPort *port = GetInputPort(port_id);
  return port ? port->enabled : false;
}

bool ArtNetNodeImpl::SetOutputPortUniverse(uint8_t port_id,
                                           uint8_t universe_id) {
  OutputPort *port = GetOutputPort(port_id);
  if (!port) {
    return false;
  }

  if (port->enabled && (port->universe_address & 0xf) == (universe_id & 0xf)) {
    return true;
  }

  port->universe_address = (
      (universe_id & 0x0f) | (port->universe_address & 0xf0));
  port->enabled = true;
  return SendPollReplyIfRequired();
}

uint8_t ArtNetNodeImpl::GetOutputPortUniverse(uint8_t port_id) {
  OutputPort *port = GetOutputPort(port_id);
  return port ? port->universe_address : 0;
}

void ArtNetNodeImpl::DisableOutputPort(uint8_t port_id) {
  OutputPort *port = GetOutputPort(port_id);
  if (!port) {
    return;
  }

  bool was_enabled = port->enabled;
  port->enabled = false;
  if (was_enabled) {
    SendPollReplyIfRequired();
  }
}

bool ArtNetNodeImpl::OutputPortState(uint8_t port_id) const {
  const OutputPort *port = GetOutputPort(port_id);
  return port ? port->enabled : false;
}

bool ArtNetNodeImpl::SetMergeMode(uint8_t port_id,
                                  artnet_merge_mode merge_mode) {
  OutputPort *port = GetOutputPort(port_id);
  if (!port) {
    return false;
  }

  port->merge_mode = merge_mode;
  return SendPollReplyIfRequired();
}

bool ArtNetNodeImpl::SendPoll() {
  if (!m_running) {
    return false;
  }

  bool send = false;
  InputPorts::const_iterator iter = m_input_ports.begin();
  for (; iter != m_input_ports.end(); ++iter) {
    send |= (*iter)->enabled;
  }

  if (!send) {
    return true;
  }

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

bool ArtNetNodeImpl::SendDMX(uint8_t port_id, const DmxBuffer &buffer) {
  InputPort *port = GetEnabledInputPort(port_id, "ArtDMX");
  if (!port) {
    return false;
  }

  if (!buffer.Size()) {
    OLA_DEBUG << "Not sending 0 length packet";
    return true;
  }

  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_DMX);
  memset(&packet.data.dmx, 0, sizeof(packet.data.dmx));

  packet.data.poll.version = HostToNetwork(ARTNET_VERSION);
  packet.data.dmx.sequence = port->sequence_number;
  packet.data.dmx.physical = port_id;
  packet.data.dmx.universe = port->PortAddress();
  packet.data.dmx.net = m_net_address;

  unsigned int buffer_size = buffer.Size();
  buffer.Get(packet.data.dmx.data, &buffer_size);

  // the dmx frame size needs to be a multiple of two, correct here if needed
  if (buffer_size % 2) {
    packet.data.dmx.data[buffer_size] = 0;
    buffer_size++;
  }
  packet.data.dmx.length[0] = buffer_size >> 8;
  packet.data.dmx.length[1] = buffer_size & 0xff;

  unsigned int size = sizeof(packet.data.dmx) - DMX_UNIVERSE_SIZE + buffer_size;

  bool sent_ok = false;
  if (port->subscribed_nodes.size() >= m_broadcast_threshold ||
      m_always_broadcast) {
    sent_ok = SendPacket(
        packet,
        size,
        m_use_limited_broadcast_address ?
        IPV4Address::Broadcast() :
        m_interface.bcast_address);
    port->sequence_number++;
  } else {
    map<IPV4Address, TimeStamp>::iterator iter = port->subscribed_nodes.begin();
    TimeStamp last_heard_threshold = (
        *m_ss->WakeUpTime() - TimeInterval(NODE_TIMEOUT, 0));
    while (iter != port->subscribed_nodes.end()) {
      // if this node has timed out, remove it from the set
      if (iter->second < last_heard_threshold) {
        port->subscribed_nodes.erase(iter++);
        continue;
      }
      sent_ok |= SendPacket(packet, size, iter->first);
      ++iter;
    }

    if (port->subscribed_nodes.empty()) {
      OLA_DEBUG << "Suppressing data transmit due to no active nodes for "
                   "universe "
                << static_cast<int>(port->PortAddress());
      sent_ok = true;
    } else {
      // We sent at least one packet, increment the sequence number
      port->sequence_number++;
    }
  }

  if (!sent_ok) {
    OLA_WARN << "Failed to send Art-Net DMX packet";
  }
  return sent_ok;
}

void ArtNetNodeImpl::RunFullDiscovery(uint8_t port_id,
                                      RDMDiscoveryCallback *callback) {
  InputPort *port = GetEnabledInputPort(port_id, "ArtTodControl");
  if (!port) {
    UIDSet uids;
    callback->Run(uids);
    return;
  }

  if (!StartDiscoveryProcess(port, callback)) {
    return;
  }

  OLA_DEBUG << "Sending ArtTodControl";
  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_TODCONTROL);
  memset(&packet.data.tod_control, 0, sizeof(packet.data.tod_control));
  packet.data.tod_control.version = HostToNetwork(ARTNET_VERSION);
  packet.data.tod_control.net = m_net_address;
  packet.data.tod_control.command = TOD_FLUSH_COMMAND;
  packet.data.tod_control.address = port->PortAddress();
  unsigned int size = sizeof(packet.data.tod_control);
  if (!SendPacket(packet, size, m_interface.bcast_address)) {
    port->RunDiscoveryCallback();
  }
}

void ArtNetNodeImpl::RunIncrementalDiscovery(
    uint8_t port_id,
    RDMDiscoveryCallback *callback) {
  InputPort *port = GetEnabledInputPort(port_id, "ArtTodRequest");
  if (!port) {
    UIDSet uids;
    callback->Run(uids);
    return;
  }

  if (!StartDiscoveryProcess(port, callback)) {
    return;
  }

  OLA_DEBUG << "Sending ArtTodRequest for address "
            << static_cast<int>(port->PortAddress());
  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_TODREQUEST);
  memset(&packet.data.tod_request, 0, sizeof(packet.data.tod_request));
  packet.data.tod_request.version = HostToNetwork(ARTNET_VERSION);
  packet.data.tod_request.net = m_net_address;
  packet.data.tod_request.address_count = 1;  // only one universe address
  packet.data.tod_request.addresses[0] = port->PortAddress();
  unsigned int size = sizeof(packet.data.tod_request);
  if (!SendPacket(packet, size, m_interface.bcast_address)) {
    port->RunDiscoveryCallback();
  }
}

void ArtNetNodeImpl::SendRDMRequest(uint8_t port_id,
                                    RDMRequest *request_ptr,
                                    RDMCallback *on_complete) {
  auto_ptr<RDMRequest> request(request_ptr);
  if (request->CommandClass() == RDMCommand::DISCOVER_COMMAND) {
    RunRDMCallback(on_complete, ola::rdm::RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED);
    return;
  }

  InputPort *port = GetEnabledInputPort(port_id, "ArtRDM");
  if (!port) {
    RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
    return;
  }

  if (port->rdm_request_callback) {
    OLA_FATAL << "Previous request hasn't completed yet, dropping request";
    RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
    return;
  }

  port->rdm_ip_destination = m_interface.bcast_address;
  const UID &uid_destination = request->DestinationUID();
  uid_map::const_iterator iter = port->uids.find(uid_destination);
  if (iter == port->uids.end()) {
    if (!uid_destination.IsBroadcast()) {
      OLA_WARN << "Couldn't find " << uid_destination
               << " in the uid map, broadcasting packet";
    }
  } else {
    port->rdm_ip_destination = iter->second.first;
  }

  port->rdm_request_callback = on_complete;
  port->pending_request = request.release();
  bool r = SendRDMCommand(*port->pending_request,
                          port->rdm_ip_destination,
                          port->PortAddress());

  if (r && !uid_destination.IsBroadcast()) {
    port->rdm_send_timeout = m_ss->RegisterSingleTimeout(
      RDM_REQUEST_TIMEOUT_MS,
      ola::NewSingleCallback(this, &ArtNetNodeImpl::TimeoutRDMRequest, port));
  } else {
    delete port->pending_request;
    port->pending_request = NULL;
    port->rdm_request_callback = NULL;
    RunRDMCallback(
        on_complete,
        uid_destination.IsBroadcast() ? ola::rdm::RDM_WAS_BROADCAST :
                                        ola::rdm::RDM_FAILED_TO_SEND);
  }
}

bool ArtNetNodeImpl::SetUnsolicitedUIDSetHandler(
    uint8_t port_id,
    ola::Callback1<void, const ola::rdm::UIDSet&> *tod_callback) {
  InputPort *port = GetInputPort(port_id);
  if (port) {
    port->SetTodCallback(tod_callback);
  }
  return port;
}

void ArtNetNodeImpl::GetSubscribedNodes(
    uint8_t port_id,
    vector<IPV4Address> *node_addresses) {
  InputPort *port = GetInputPort(port_id);
  if (!port) {
    return;
  }

  map<IPV4Address, TimeStamp> &subscribed_nodes = port->subscribed_nodes;
  map<IPV4Address, TimeStamp>::const_iterator iter = subscribed_nodes.begin();
  for (; iter != subscribed_nodes.end(); ++iter) {
    TimeStamp last_heard_threshold = (
        *m_ss->WakeUpTime() - TimeInterval(NODE_TIMEOUT, 0));
    if (iter->second >= last_heard_threshold) {
      node_addresses->push_back(iter->first);
    }
  }
}

bool ArtNetNodeImpl::SetDMXHandler(uint8_t port_id,
                                   DmxBuffer *buffer,
                                   Callback0<void> *on_data) {
  OutputPort *port = GetOutputPort(port_id);
  if (!port) {
    return false;
  }

  if (port->on_data) {
    delete m_output_ports[port_id].on_data;
  }
  port->buffer = buffer;
  port->on_data = on_data;
  return true;
}

bool ArtNetNodeImpl::SendTod(uint8_t port_id, const UIDSet &uid_set) {
  OutputPort *port = GetEnabledOutputPort(port_id, "ArtTodData");
  if (!port) {
    return false;
  }

  OLA_DEBUG << "Sending ArtTodRequest";
  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_TODDATA);
  memset(&packet.data.tod_data, 0, sizeof(packet.data.tod_data));
  packet.data.tod_data.version = HostToNetwork(ARTNET_VERSION);
  packet.data.tod_data.rdm_version = RDM_VERSION;
  packet.data.tod_data.port = 1 + port_id;
  packet.data.tod_request.net = m_net_address;
  packet.data.tod_data.address = port->universe_address;
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

bool ArtNetNodeImpl::SetOutputPortRDMHandlers(
    uint8_t port_id,
    ola::Callback0<void> *on_discover,
    ola::Callback0<void> *on_flush,
    ola::Callback2<void, RDMRequest*, RDMCallback*> *on_rdm_request) {
  OutputPort *port = GetOutputPort(port_id);
  if (!port) {
    return false;
  }

  if (port->on_discover) {
    delete port->on_discover;
  }
  if (port->on_flush) {
    delete port->on_flush;
  }
  if (port->on_rdm_request) {
    delete port->on_rdm_request;
  }
  port->on_discover = on_discover;
  port->on_flush = on_flush;
  port->on_rdm_request = on_rdm_request;
  return true;
}

bool ArtNetNodeImpl::SendTimeCode(const ola::timecode::TimeCode &timecode) {
  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_TIME_CODE);
  memset(&packet.data.timecode, 0, sizeof(packet.data.timecode));
  packet.data.timecode.version = HostToNetwork(ARTNET_VERSION);

  packet.data.timecode.frames = timecode.Frames();
  packet.data.timecode.seconds = timecode.Seconds();
  packet.data.timecode.minutes = timecode.Minutes();
  packet.data.timecode.hours = timecode.Hours();
  packet.data.timecode.type = timecode.Type();
  if (!SendPacket(packet,
                  sizeof(packet.data.timecode),
                  m_interface.bcast_address)) {
    OLA_INFO << "Failed to send ArtTimeCode";
    return false;
  }
  return true;
}

void ArtNetNodeImpl::SocketReady() {
  artnet_packet packet;
  ssize_t packet_size = sizeof(packet);
  ola::network::IPV4SocketAddress source;

  if (!m_socket->RecvFrom(reinterpret_cast<uint8_t*>(&packet),
                          &packet_size,
                          &source)) {
    return;
  }

  HandlePacket(source.Host(), packet, packet_size);
}

bool ArtNetNodeImpl::SendPollIfAllowed() {
  if (!m_running) {
    return true;
  }

  if (m_in_configuration_mode) {
    m_artpoll_required = true;
  } else {
    return SendPoll();
  }
  return true;
}

bool ArtNetNodeImpl::SendPollReplyIfRequired() {
  if (m_running && m_send_reply_on_change) {
    if (m_in_configuration_mode) {
      m_artpollreply_required = true;
    } else {
      m_unsolicited_replies++;
      return SendPollReply(m_interface.bcast_address);
    }
  }
  return true;
}

bool ArtNetNodeImpl::SendPollReply(const IPV4Address &destination) {
  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_REPLY);
  memset(&packet.data.reply, 0, sizeof(packet.data.reply));

  m_interface.ip_address.Get(packet.data.reply.ip);
  packet.data.reply.port = HostToLittleEndian(ARTNET_PORT);
  packet.data.reply.net_address = m_net_address;
  packet.data.reply.subnet_address = m_output_ports[0].universe_address >> 4;
  packet.data.reply.oem = HostToNetwork(OEM_CODE);
  packet.data.reply.status1 = 0xd2;  // normal indicators, rdm enabled
  packet.data.reply.esta_id = HostToLittleEndian(OPEN_LIGHTING_ESTA_CODE);
  strings::StrNCopy(packet.data.reply.short_name,
                    m_short_name.data());
  strings::StrNCopy(packet.data.reply.long_name,
                    m_long_name.data());

  std::ostringstream str;
  str << "#0001 [" << m_unsolicited_replies << "] OLA";
  CopyToFixedLengthBuffer(str.str(), packet.data.reply.node_report,
                          arraysize(packet.data.reply.node_report));
  packet.data.reply.number_ports[1] = ARTNET_MAX_PORTS;
  for (unsigned int i = 0; i < ARTNET_MAX_PORTS; i++) {
    InputPort *iport = GetInputPort(i, false);
    packet.data.reply.port_types[i] = iport ? 0xc0 : 0x80;
    packet.data.reply.good_input[i] = iport && iport->enabled ? 0x0 : 0x8;
    packet.data.reply.sw_in[i] = iport ? iport->PortAddress() : 0;

    packet.data.reply.good_output[i] = (
        (m_output_ports[i].enabled ? 0x80 : 0x00) |
        (m_output_ports[i].merge_mode == ARTNET_MERGE_LTP ? 0x2 : 0x0) |
        (m_output_ports[i].is_merging ? 0x8 : 0x0));
    packet.data.reply.sw_out[i] = m_output_ports[i].universe_address;
  }
  packet.data.reply.style = NODE_CODE;
  m_interface.hw_address.Get(packet.data.reply.mac);
  m_interface.ip_address.Get(packet.data.reply.bind_ip);
  // maybe set status2 here if the web UI is enabled
  packet.data.reply.status2 = 0x08;  // node supports 15 bit port addresses
  if (!SendPacket(packet, sizeof(packet.data.reply), destination)) {
    OLA_INFO << "Failed to send ArtPollReply";
    return false;
  }
  return true;
}

bool ArtNetNodeImpl::SendIPReply(const IPV4Address &destination) {
  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_REPLY);
  memset(&packet.data.ip_reply, 0, sizeof(packet.data.ip_reply));
  packet.data.ip_reply.version = HostToNetwork(ARTNET_VERSION);

  m_interface.ip_address.Get(packet.data.ip_reply.ip);
  m_interface.subnet_mask.Get(packet.data.ip_reply.subnet);
  packet.data.ip_reply.port = HostToLittleEndian(ARTNET_PORT);

  if (!SendPacket(packet, sizeof(packet.data.ip_reply), destination)) {
    OLA_INFO << "Failed to send ArtIpProgReply";
    return false;
  }
  return true;
}

void ArtNetNodeImpl::HandlePacket(const IPV4Address &source_address,
                                  const artnet_packet &packet,
                                  unsigned int packet_size) {
  unsigned int header_size = sizeof(packet) - sizeof(packet.data);

  if (packet_size <= header_size) {
    OLA_WARN << "Skipping small Art-Net packet received, size=" << packet_size;
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
    case ARTNET_SYNC:
      // TODO(Someone): Implement me, not currently implemented.
      OLA_DEBUG << "ArtSync input not currently supported";
      break;
    case ARTNET_RDM_SUB:
      // TODO(Someone): Implement me, not currently implemented.
      OLA_DEBUG << "ArtRDMSub input not currently supported";
      break;
    case ARTNET_TIME_CODE:
      // TODO(Someone): Implement me, not currently implemented.
      OLA_DEBUG << "ArtTimeCode input not currently supported";
      break;
    default:
      OLA_INFO << "Art-Net got unknown packet " << std::hex
               << LittleEndianToHost(packet.op_code);
  }
}

void ArtNetNodeImpl::HandlePollPacket(const IPV4Address &source_address,
                                      const artnet_poll_t &packet,
                                      unsigned int packet_size) {
  if (!CheckPacketSize(source_address,
                       "ArtPoll",
                       packet_size,
                       sizeof(packet))) {
    return;
  }

  if (!CheckPacketVersion(source_address, "ArtPoll", packet.version)) {
    return;
  }

  m_send_reply_on_change = packet.talk_to_me & 0x02;
  // It's unclear if this should be broadcast or unicast, stick with broadcast
  SendPollReply(m_interface.bcast_address);
  (void) source_address;
}

void ArtNetNodeImpl::HandleReplyPacket(const IPV4Address &source_address,
                                       const artnet_reply_t &packet,
                                       unsigned int packet_size) {
  if (m_interface.ip_address == source_address) {
    return;
  }

  // Older versions don't have the bind_ip and the extra filler, make sure we
  // support these
  unsigned int minimum_reply_size = (
      sizeof(packet) -
      sizeof(packet.filler) -
      sizeof(packet.status2) -
      sizeof(packet.bind_index) -
      sizeof(packet.bind_ip));
  if (!CheckPacketSize(source_address, "ArtPollReply", packet_size,
                       minimum_reply_size)) {
    return;
  }

  if (packet.net_address != m_net_address) {
    OLA_DEBUG << "Received ArtPollReply for net "
              << static_cast<int>(packet.net_address)
              << " which doesn't match our net address "
              << static_cast<int>(m_net_address) << ", discarding";
    return;
  }

  // Update the subscribed nodes list
  unsigned int port_limit = std::min((uint8_t) ARTNET_MAX_PORTS,
                                     packet.number_ports[1]);
  for (unsigned int i = 0; i < port_limit; i++) {
    if (packet.port_types[i] & 0x80) {
      // port is of type output
      uint8_t universe_id = packet.sw_out[i];
      InputPorts::iterator iter = m_input_ports.begin();
      for (; iter != m_input_ports.end(); ++iter) {
        if ((*iter)->enabled && (*iter)->PortAddress() == universe_id) {
          STLReplace(&(*iter)->subscribed_nodes, source_address,
                     *m_ss->WakeUpTime());
        }
      }
    }
  }
}

void ArtNetNodeImpl::HandleDataPacket(const IPV4Address &source_address,
                                      const artnet_dmx_t &packet,
                                      unsigned int packet_size) {
  // The data section needs to be at least 2 bytes according to the spec
  unsigned int header_size = sizeof(artnet_dmx_t) - DMX_UNIVERSE_SIZE;
  if (!CheckPacketSize(source_address,
                       "ArtDmx",
                       packet_size,
                       header_size + 2)) {
    return;
  }

  if (!CheckPacketVersion(source_address, "ArtDmx", packet.version)) {
    return;
  }

  if (packet.net != m_net_address) {
    OLA_DEBUG << "Received ArtDmx for net " << static_cast<int>(packet.net)
              << " which doesn't match our net address "
              << static_cast<int>(m_net_address) << ", discarding";
    return;
  }

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

void ArtNetNodeImpl::HandleTodRequest(const IPV4Address &source_address,
                                      const artnet_todrequest_t &packet,
                                      unsigned int packet_size) {
  unsigned int header_size = sizeof(packet) - sizeof(packet.addresses);
  if (!CheckPacketSize(source_address, "ArtTodRequest", packet_size,
                       header_size)) {
    return;
  }

  if (!CheckPacketVersion(source_address, "ArtTodRequest", packet.version)) {
    return;
  }

  if (packet.net != m_net_address) {
    OLA_DEBUG << "Received ArtTodRequest for net "
              << static_cast<int>(packet.net)
              << " which doesn't match our net address "
              << static_cast<int>(m_net_address) << ", discarding";
    return;
  }

  if (packet.command) {
    OLA_INFO << "ArtTodRequest received but command field was "
             << static_cast<int>(packet.command);
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

void ArtNetNodeImpl::HandleTodData(const IPV4Address &source_address,
                                   const artnet_toddata_t &packet,
                                   unsigned int packet_size) {
  unsigned int expected_size = sizeof(packet) - sizeof(packet.tod);
  if (!CheckPacketSize(source_address, "ArtTodData", packet_size,
                       expected_size)) {
    return;
  }

  if (!CheckPacketVersion(source_address, "ArtTodData", packet.version)) {
    return;
  }

  if (packet.rdm_version != RDM_VERSION) {
    OLA_WARN << "Dropping non standard RDM version: "
             << static_cast<int>(packet.rdm_version);
    return;
  }

  if (packet.net != m_net_address) {
    OLA_DEBUG << "Received ArtTodData for net " << static_cast<int>(packet.net)
              << " which doesn't match our net address "
              << static_cast<int>(m_net_address) << ", discarding";
    return;
  }

  if (packet.command_response) {
    OLA_WARN << "Command response " << ToHex(packet.command_response)
             << " != 0x0";
    return;
  }

  InputPorts::iterator iter = m_input_ports.begin();
  for (; iter != m_input_ports.end(); ++iter) {
    if ((*iter)->enabled && (*iter)->PortAddress() == packet.address) {
      UpdatePortFromTodPacket(*iter, source_address, packet, packet_size);
    }
  }
}

void ArtNetNodeImpl::HandleTodControl(const IPV4Address &source_address,
                                      const artnet_todcontrol_t &packet,
                                      unsigned int packet_size) {
  if (!CheckPacketSize(source_address, "ArtTodControl", packet_size,
                       sizeof(packet))) {
    return;
  }

  if (!CheckPacketVersion(source_address, "ArtTodControl", packet.version)) {
    return;
  }

  if (packet.net != m_net_address) {
    OLA_DEBUG << "Received ArtTodControl for net "
              << static_cast<int>(packet.net)
              << " which doesn't match our net address "
              << static_cast<int>(m_net_address) << ", discarding";
    return;
  }

  if (packet.command != TOD_FLUSH_COMMAND) {
    return;
  }

  for (unsigned int port_id = 0; port_id < ARTNET_MAX_PORTS; port_id++) {
    if (m_output_ports[port_id].enabled &&
        m_output_ports[port_id].universe_address == packet.address &&
        m_output_ports[port_id].on_flush) {
      m_output_ports[port_id].on_flush->Run();
    }
  }
}

void ArtNetNodeImpl::HandleRdm(const IPV4Address &source_address,
                               const artnet_rdm_t &packet,
                               unsigned int packet_size) {
  unsigned int header_size = sizeof(packet) - ARTNET_MAX_RDM_DATA;
  if (!CheckPacketSize(source_address, "ArtRDM", packet_size, header_size)) {
    return;
  }

  if (!CheckPacketVersion(source_address, "ArtRDM", packet.version)) {
    return;
  }

  if (packet.rdm_version != RDM_VERSION) {
    OLA_INFO << "Dropping non standard RDM version: "
             << static_cast<int>(packet.rdm_version);
    return;
  }

  if (packet.command) {
    OLA_WARN << "Unknown RDM command " << static_cast<int>(packet.command);
    return;
  }

  if (packet.net != m_net_address) {
    OLA_DEBUG << "Received ArtRDM for net " << static_cast<int>(packet.net)
              << " which doesn't match our net address "
              << static_cast<int>(m_net_address) << ", discarding";
    return;
  }

  unsigned int rdm_length = packet_size - header_size;
  if (!rdm_length) {
    return;
  }

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
            NewSingleCallback(this,
                              &ArtNetNodeImpl::RDMRequestCompletion,
                              source_address,
                              port_id,
                              m_output_ports[port_id].universe_address));
      }
    }
  }

  // The Art-Net packet does not include the RDM start code. Prepend that.
  RDMFrame rdm_response(packet.data, rdm_length, RDMFrame::Options(true));

  InputPorts::iterator iter = m_input_ports.begin();
  for (; iter != m_input_ports.end(); ++iter) {
    if ((*iter)->enabled && (*iter)->PortAddress() == packet.address) {
      HandleRDMResponse(*iter, rdm_response, source_address);
    }
  }
}

void ArtNetNodeImpl::RDMRequestCompletion(
    IPV4Address destination,
    uint8_t port_id,
    uint8_t universe_address,
    RDMReply *reply) {
  OutputPort *port = GetEnabledOutputPort(port_id, "ArtRDM");
  if (!port) {
    return;
  }

  if (port->universe_address == universe_address) {
    if (reply->StatusCode() == ola::rdm::RDM_COMPLETED_OK) {
      // TODO(simon): handle fragmenation here
      SendRDMCommand(*reply->Response(), destination, universe_address);
    } else if (reply->StatusCode() == ola::rdm::RDM_UNKNOWN_UID) {
      // call the on discovery handler, which will send a new TOD and
      // hopefully update the remote controller
      port->on_discover->Run();
    } else {
      OLA_WARN << "Art-Net RDM request failed with code "
               << reply->StatusCode();
    }
  } else {
    // the universe address has changed we need to drop this request
    OLA_WARN << "Art-Net Output port has changed mid request, dropping "
             << "response";
  }
}

void ArtNetNodeImpl::HandleRDMResponse(InputPort *port,
                                       const RDMFrame &frame,
                                       const IPV4Address &source_address) {
  auto_ptr<RDMReply> reply(ola::rdm::RDMReply::FromFrame(frame));

  // Without a valid response, we don't know which request this matches. This
  // makes Art-Net rather useless for RDM regression testing
  if (!reply->Response()) {
    return;
  }

  if (!port->pending_request) {
    return;
  }

  const RDMRequest *request = port->pending_request;
  if (request->SourceUID() != reply->Response()->DestinationUID() ||
      request->DestinationUID() != reply->Response()->SourceUID()) {
    OLA_INFO << "Got response from/to unexpected UID: req "
             << request->SourceUID() << " -> " << request->DestinationUID()
             << ", res " << reply->Response()->SourceUID() << " -> "
             << reply->Response()->DestinationUID();
    return;
  }

  if (request->ParamId() != ola::rdm::PID_QUEUED_MESSAGE &&
      request->ParamId() != reply->Response()->ParamId()) {
    OLA_INFO << "Param ID mismatch, request was "
             << ToHex(request->ParamId()) << ", response was "
             << ToHex(reply->Response()->ParamId());
    return;
  }

  if (request->ParamId() != ola::rdm::PID_QUEUED_MESSAGE &&
      request->SubDevice() != ola::rdm::ALL_RDM_SUBDEVICES &&
      request->SubDevice() != reply->Response()->SubDevice()) {
    OLA_INFO << "Subdevice mismatch, request was for"
             << request->SubDevice() << ", response was "
             << reply->Response()->SubDevice();
    return;
  }

  if (request->CommandClass() == RDMCommand::GET_COMMAND &&
      reply->Response()->CommandClass() != RDMCommand::GET_COMMAND_RESPONSE &&
      request->ParamId() != ola::rdm::PID_QUEUED_MESSAGE) {
    OLA_INFO << "Invalid return CC in response to get, was "
             << ToHex(reply->Response()->CommandClass());
    return;
  }

  if (request->CommandClass() == RDMCommand::SET_COMMAND &&
      reply->Response()->CommandClass() != RDMCommand::SET_COMMAND_RESPONSE) {
    OLA_INFO << "Invalid return CC in response to set, was "
             << ToHex(reply->Response()->CommandClass());
    return;
  }

  if (port->rdm_ip_destination != m_interface.bcast_address &&
      port->rdm_ip_destination != source_address) {
    OLA_INFO << "IP address of RDM response didn't match";
    return;
  }

  // at this point we've decided it's for us
  port->pending_request = NULL;
  delete request;
  RDMCallback *callback = port->rdm_request_callback;
  port->rdm_request_callback = NULL;

  // remove the timeout
  if (port->rdm_send_timeout != ola::thread::INVALID_TIMEOUT) {
    m_ss->RemoveTimeout(port->rdm_send_timeout);
    port->rdm_send_timeout = ola::thread::INVALID_TIMEOUT;
  }

  callback->Run(reply.get());
}

void ArtNetNodeImpl::HandleIPProgram(const IPV4Address &source_address,
                                     const artnet_ip_prog_t &packet,
                                     unsigned int packet_size) {
  if (!CheckPacketSize(source_address, "ArtIpProg", packet_size,
                       sizeof(packet))) {
    return;
  }

  if (!CheckPacketVersion(source_address, "ArtIpProg", packet.version)) {
    return;
  }

  OLA_INFO << "Got ArtIpProgram, ignoring because we don't support remote "
           << "configuration";
}

void ArtNetNodeImpl::PopulatePacketHeader(artnet_packet *packet,
                                          uint16_t op_code) {
  CopyToFixedLengthBuffer(ARTNET_ID, reinterpret_cast<char*>(packet->id),
                          arraysize(packet->id));
  packet->op_code = HostToLittleEndian(op_code);
}

bool ArtNetNodeImpl::SendPacket(const artnet_packet &packet,
                                unsigned int size,
                                const IPV4Address &ip_destination) {
  size += sizeof(packet.id) + sizeof(packet.op_code);
  unsigned int bytes_sent = m_socket->SendTo(
      reinterpret_cast<const uint8_t*>(&packet),
      size,
      IPV4SocketAddress(ip_destination, ARTNET_PORT));

  if (bytes_sent != size) {
    OLA_INFO << "Only sent " << bytes_sent << " of " << size;
    return false;
  }
  return true;
}

void ArtNetNodeImpl::TimeoutRDMRequest(InputPort *port) {
  OLA_INFO << "RDM Request timed out.";
  port->rdm_send_timeout = ola::thread::INVALID_TIMEOUT;
  delete port->pending_request;
  port->pending_request = NULL;
  RDMCallback *callback = port->rdm_request_callback;
  port->rdm_request_callback = NULL;
  RunRDMCallback(callback, ola::rdm::RDM_TIMEOUT);
}

bool ArtNetNodeImpl::SendRDMCommand(const RDMCommand &command,
                                    const IPV4Address &destination,
                                    uint8_t universe) {
  artnet_packet packet;
  PopulatePacketHeader(&packet, ARTNET_RDM);
  memset(&packet.data.rdm, 0, sizeof(packet.data.rdm));
  packet.data.rdm.version = HostToNetwork(ARTNET_VERSION);
  packet.data.rdm.rdm_version = RDM_VERSION;
  packet.data.rdm.net = m_net_address;
  packet.data.rdm.address = universe;
  unsigned int rdm_size = ARTNET_MAX_RDM_DATA;
  if (!RDMCommandSerializer::Pack(command, packet.data.rdm.data, &rdm_size)) {
    OLA_WARN << "Failed to construct RDM command";
    return false;
  }
  unsigned int packet_size = sizeof(packet.data.rdm) - ARTNET_MAX_RDM_DATA +
      rdm_size;
  return SendPacket(packet, packet_size, destination);
}

void ArtNetNodeImpl::UpdatePortFromSource(OutputPort *port,
                                          const DMXSource &source) {
  TimeStamp merge_time_threshold = (
      *m_ss->WakeUpTime() - TimeInterval(MERGE_TIMEOUT, 0));
  // the index of the first empty slot, of MAX_MERGE_SOURCES if we're already
  // tracking MAX_MERGE_SOURCES sources.
  unsigned int first_empty_slot = MAX_MERGE_SOURCES;
  // the index for this source, or MAX_MERGE_SOURCES if it wasn't found
  unsigned int source_slot = MAX_MERGE_SOURCES;
  unsigned int active_sources = 0;

  // locate the source within the list of tracked sources, also find the first
  // empty source location in case this source is new, and timeout any sources
  // we haven't heard from.
  for (unsigned int i = 0; i < MAX_MERGE_SOURCES; i++) {
    if (port->sources[i].address == source.address) {
      source_slot = i;
      continue;
    }

    // timeout old sources
    if (port->sources[i].timestamp < merge_time_threshold) {
      port->sources[i].address = IPV4Address();
    }

    if (!port->sources[i].address.IsWildcard()) {
      active_sources++;
    } else if (i < first_empty_slot) {
      first_empty_slot = i;
    }
  }

  if (source_slot == MAX_MERGE_SOURCES) {
    // this is a new source
    if (first_empty_slot == MAX_MERGE_SOURCES) {
      // No room at the inn
      OLA_WARN << "Max merge sources reached, ignoring";
      return;
    }
    if (active_sources == 0) {
      port->is_merging = false;
    } else {
      OLA_INFO << "Entered merge mode for universe "
               << static_cast<int>(port->universe_address);
      port->is_merging = true;
      SendPollReplyIfRequired();
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
      if (!port->sources[i].address.IsWildcard()) {
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

bool ArtNetNodeImpl::CheckPacketVersion(const IPV4Address &source_address,
                                        const string &packet_type,
                                        uint16_t version) {
  if (NetworkToHost(version) != ARTNET_VERSION) {
    OLA_INFO << packet_type << " version mismatch, was "
             << NetworkToHost(version) << " from " << source_address;
    return false;
  }
  return true;
}

bool ArtNetNodeImpl::CheckPacketSize(const IPV4Address &source_address,
                                     const string &packet_type,
                                     unsigned int actual_size,
                                     unsigned int expected_size) {
  if (actual_size < expected_size) {
    OLA_INFO << packet_type << " from " << source_address
             << " was too small, got " << actual_size
             << " required at least " << expected_size;
    return false;
  }
  return true;
}

ArtNetNodeImpl::InputPort *ArtNetNodeImpl::GetInputPort(uint8_t port_id,
                                                        bool warn) {
  if (port_id >= m_input_ports.size()) {
    if (warn) {
      OLA_WARN << "Port index out of bounds: "
               << static_cast<int>(port_id) << " >= " << m_input_ports.size();
    }
    return NULL;
  }
  return m_input_ports[port_id];
}

const ArtNetNodeImpl::InputPort *ArtNetNodeImpl::GetInputPort(
    uint8_t port_id) const {
  if (port_id >= m_input_ports.size()) {
    OLA_WARN << "Port index out of bounds: "
             << static_cast<int>(port_id) << " >= " << m_input_ports.size();
    return NULL;
  }
  return m_input_ports[port_id];
}

ArtNetNodeImpl::InputPort *ArtNetNodeImpl::GetEnabledInputPort(
    uint8_t port_id,
    const string &action) {
  if (!m_running) {
    return NULL;
  }

  InputPort *port = GetInputPort(port_id);
  bool ok = port ? port->enabled : false;
  if (!ok) {
    OLA_INFO << "Attempt to send " << action << " on an inactive port";
  }
  return ok ? port : NULL;
}

ArtNetNodeImpl::OutputPort *ArtNetNodeImpl::GetOutputPort(uint8_t port_id) {
  if (port_id >= ARTNET_MAX_PORTS) {
    OLA_WARN << "Port index out of bounds: "
             << static_cast<int>(port_id) << " >= " << ARTNET_MAX_PORTS;
    return NULL;
  }
  return &m_output_ports[port_id];
}

const ArtNetNodeImpl::OutputPort *ArtNetNodeImpl::GetOutputPort(
    uint8_t port_id) const {
  if (port_id >= ARTNET_MAX_PORTS) {
    OLA_WARN << "Port index out of bounds: "
             << static_cast<int>(port_id) << " >= " << ARTNET_MAX_PORTS;
    return NULL;
  }
  return &m_output_ports[port_id];
}

ArtNetNodeImpl::OutputPort *ArtNetNodeImpl::GetEnabledOutputPort(
    uint8_t port_id,
    const string &action) {
  if (!m_running) {
    return NULL;
  }

  OutputPort *port = GetOutputPort(port_id);
  bool ok = port ? port->enabled : false;
  if (!ok) {
    OLA_INFO << "Attempt to send " << action << " on an inactive port";
  }
  return ok ? port : NULL;
}

bool ArtNetNodeImpl::InitNetwork() {
  if (!m_socket->Init()) {
    OLA_WARN << "Socket init failed";
    return false;
  }

  if (!m_socket->Bind(IPV4SocketAddress(IPV4Address::WildCard(),
                                        ARTNET_PORT))) {
    return false;
  }

  if (!m_socket->EnableBroadcast()) {
    OLA_WARN << "Failed to enable broadcasting";
    return false;
  }

  m_socket->SetOnData(NewCallback(this, &ArtNetNodeImpl::SocketReady));
  m_ss->AddReadDescriptor(m_socket.get());
  return true;
}

void ArtNetNodeImpl::UpdatePortFromTodPacket(InputPort *port,
                                             const IPV4Address &source_address,
                                             const artnet_toddata_t &packet,
                                             unsigned int packet_size) {
  unsigned int tod_size = packet_size - (sizeof(packet) - sizeof(packet.tod));
  unsigned int uid_count = std::min(tod_size / ola::rdm::UID::UID_SIZE,
                                    (unsigned int) packet.uid_count);

  OLA_DEBUG << "Got TOD data packet with " << uid_count << " UIDs";
  uid_map &port_uids = port->uids;
  UIDSet uid_set;

  for (unsigned int i = 0; i < uid_count; i++) {
    UID uid(packet.tod[i]);
    uid_set.AddUID(uid);
    uid_map::iterator iter = port_uids.find(uid);
    if (iter == port_uids.end()) {
      port_uids[uid] = std::pair<IPV4Address, uint8_t>(source_address, 0);
    } else {
      if (iter->second.first != source_address) {
        OLA_WARN << "UID " << uid << " changed from "
                 << iter->second.first << " to " << source_address;
        iter->second.first = source_address;
      }
      iter->second.second = 0;
    }
  }

  // If this is the one and only block from this node, we can remove all uids
  // that don't appear in it.
  // There is a bug in Art-Net nodes where sometimes UidCount > UidTotal.
  if (uid_count >= NetworkToHost(packet.uid_total)) {
    uid_map::iterator iter = port_uids.begin();
    while (iter != port_uids.end()) {
      if (iter->second.first == source_address &&
          !uid_set.Contains(iter->first)) {
        port_uids.erase(iter++);
      } else {
        ++iter;
      }
    }

    // mark this node as complete
    if (port->discovery_node_set.erase(source_address)) {
      // if the set is now 0, and it was non-0 initially and we have a
      // discovery_callback, then we can short circuit the discovery
      // process
      if (port->discovery_node_set.empty() && port->discovery_callback) {
        m_ss->RemoveTimeout(port->discovery_timeout);
        ReleaseDiscoveryLock(port);
      }
    }
  }

  // Removing uids from multi-block messages is much harder as you need to
  // consider dropped packets. For the moment we rely on the
  // RDM_MISSED_TODDATA_LIMIT to clean these up.
  // TODO(simon): figure this out sometime

  // if we're not in the middle of a discovery process, send an unsolicited
  // update if we have a callback
  if (!port->discovery_callback)
    port->RunTodCallback();
}

bool ArtNetNodeImpl::StartDiscoveryProcess(InputPort *port,
                                           RDMDiscoveryCallback *callback) {
  if (port->discovery_callback) {
    OLA_FATAL << "Art-Net UID discovery already running, something has gone "
                 "wrong with the DiscoverableQueueingRDMController.";
    port->RunTodCallback();
    return false;
  }

  port->discovery_callback = callback;
  port->IncrementUIDCounts();

  // populate the discovery set with the nodes we know about, this allows us to
  // 'finish' the discovery process when we receive ArtTod packets from all
  // these nodes. If ArtTod packets arrive after discovery completes, we'll
  // call the unsolicited handler
  port->discovery_node_set.clear();
  map<IPV4Address, TimeStamp>::const_iterator node_iter =
      port->subscribed_nodes.begin();
  for (; node_iter != port->subscribed_nodes.end(); node_iter++)
    port->discovery_node_set.insert(node_iter->first);

  port->discovery_timeout = m_ss->RegisterSingleTimeout(
      RDM_TOD_TIMEOUT_MS,
      ola::NewSingleCallback(this, &ArtNetNodeImpl::ReleaseDiscoveryLock,
                             port));
  return true;
}

void ArtNetNodeImpl::ReleaseDiscoveryLock(InputPort *port) {
  OLA_INFO << "Art-Net RDM discovery complete";
  port->discovery_timeout = ola::thread::INVALID_TIMEOUT;
  port->discovery_node_set.clear();

  // delete all uids that have reached the max count
  uid_map::iterator iter = port->uids.begin();
  while (iter != port->uids.end()) {
    if (iter->second.second == RDM_MISSED_TODDATA_LIMIT) {
      port->uids.erase(iter++);
    } else {
      ++iter;
    }
  }

  port->RunDiscoveryCallback();
}

ArtNetNode::ArtNetNode(const ola::network::Interface &iface,
                       ola::io::SelectServerInterface *ss,
                       const ArtNetNodeOptions &options,
                       ola::network::UDPSocketInterface *socket):
    m_impl(iface, ss, options, socket) {
  for (unsigned int i = 0; i < options.input_port_count; i++) {
    ArtNetNodeImplRDMWrapper *wrapper = new ArtNetNodeImplRDMWrapper(&m_impl,
                                                                     i);
    m_wrappers.push_back(wrapper);
    m_controllers.push_back(new ola::rdm::DiscoverableQueueingRDMController(
        wrapper, options.rdm_queue_size));
  }
}


ArtNetNode::~ArtNetNode() {
  STLDeleteElements(&m_controllers);
  STLDeleteElements(&m_wrappers);
}

void ArtNetNode::RunFullDiscovery(uint8_t port_id,
                                  RDMDiscoveryCallback *callback) {
  if (!CheckInputPortId(port_id)) {
    ola::rdm::UIDSet uids;
    callback->Run(uids);
  } else {
    m_controllers[port_id]->RunFullDiscovery(callback);
  }
}

void ArtNetNode::RunIncrementalDiscovery(uint8_t port_id,
                                         RDMDiscoveryCallback *callback) {
  if (!CheckInputPortId(port_id)) {
    ola::rdm::UIDSet uids;
    callback->Run(uids);
  } else {
    m_controllers[port_id]->RunIncrementalDiscovery(callback);
  }
}

void ArtNetNode::SendRDMRequest(uint8_t port_id, RDMRequest *request,
                                RDMCallback *on_complete) {
  if (!CheckInputPortId(port_id)) {
    RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
    delete request;
  } else {
    m_controllers[port_id]->SendRDMRequest(request, on_complete);
  }
}

bool ArtNetNode::CheckInputPortId(uint8_t port_id) {
  if (port_id >= m_controllers.size()) {
    OLA_WARN << "Port index out of bounds: " << static_cast<int>(port_id)
             << " >= " << m_controllers.size();
    return false;
  }
  return true;
}
}  // namespace artnet
}  // namespace plugin
}  // namespace ola
