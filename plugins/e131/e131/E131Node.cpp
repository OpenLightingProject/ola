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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * E131Node.cpp
 * A E1.31 node
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string.h>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/network/InterfacePicker.h"
#include "plugins/e131/e131/E131Node.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::Callback0;
using ola::DmxBuffer;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using std::map;
using std::string;


/*
 * Create a new E1.31 node
 * @param ip_address the IP address to prefer to listen on
 * @param cid, the CID to use, if not provided we generate one
 * @param use_rev2 send using Rev2 rather than the final standard
 * @param ignore_preview ignore received data with the preview bit set
 * @param dscp_value the DSCP value to tag outgoing packets with
 * @param port the UDP port to bind to, defaults to ACN_PORT
 */
E131Node::E131Node(const string &ip_address,
                   const CID &cid,
                   bool use_rev2,
                   bool ignore_preview,
                   uint8_t dscp_value,
                   uint16_t port)
    : m_preferred_ip(ip_address),
      m_cid(cid),
      m_use_rev2(use_rev2),
      m_dscp(dscp_value),
      m_udp_port(port),
      m_root_sender(m_cid),
      m_e131_sender(&m_socket, &m_root_sender),
      m_dmp_inflator(ignore_preview),
      m_incoming_udp_transport(&m_socket, &m_root_inflator),
      m_send_buffer(NULL) {

  if (!m_use_rev2) {
    // Allocate a buffer for the dmx data + start code
    m_send_buffer = new uint8_t[DMX_UNIVERSE_SIZE + 1];
    m_send_buffer[0] = 0;  // start code is 0
  }

  // setup all the inflators
  m_root_inflator.AddInflator(&m_e131_inflator);
  m_root_inflator.AddInflator(&m_e131_rev2_inflator);
  m_e131_inflator.AddInflator(&m_dmp_inflator);
  m_e131_rev2_inflator.AddInflator(&m_dmp_inflator);
}


/*
 * Cleanup
 */
E131Node::~E131Node() {
  // remove handlers for all universes. This also leaves the multicast groups.
  vector<unsigned int> universes;
  m_dmp_inflator.RegisteredUniverses(&universes);
  vector<unsigned int>::const_iterator iter = universes.begin();
  for (; iter != universes.end(); ++iter) {
    RemoveHandler(*iter);
  }

  Stop();
  if (m_send_buffer)
    delete[] m_send_buffer;
}


/*
 * Start this node
 */
bool E131Node::Start() {
  ola::network::InterfacePicker *picker =
    ola::network::InterfacePicker::NewPicker();
  if (!picker->ChooseInterface(&m_interface, m_preferred_ip)) {
    OLA_INFO << "Failed to find an interface";
    delete picker;
    return false;
  }
  delete picker;

  if (!m_socket.Init()) {
    return false;
  }

  if (!m_socket.Bind(IPV4SocketAddress(IPV4Address::WildCard(), m_udp_port)))
    return false;

  if (!m_socket.EnableBroadcast())
    return false;

  m_socket.SetTos(m_dscp);
  m_socket.SetMulticastInterface(m_interface.ip_address);

  m_socket.SetOnData(NewCallback(&m_incoming_udp_transport,
                                 &IncomingUDPTransport::Receive));
  return true;
}


/*
 * Stop this node
 */
bool E131Node::Stop() {
  return true;
}


/*
 * Set the name for a universe
 */
bool E131Node::SetSourceName(unsigned int universe, const string &source) {
  map<unsigned int, tx_universe>::iterator iter =
      m_tx_universes.find(universe);

  if (iter == m_tx_universes.end()) {
    tx_universe *settings = SetupOutgoingSettings(universe);
    settings->source = source;
  } else {
    iter->second.source = source;
  }
  return true;
}


/*
 * Send some DMX data
 * @param universe the id of the universe to send
 * @param buffer the DMX data
 * @param priority the priority to use
 * @param preview set to true to turn on the preview bit
 * @return true if it was sent successfully, false otherwise
 */
bool E131Node::SendDMX(uint16_t universe,
                       const ola::DmxBuffer &buffer,
                       uint8_t priority,
                       bool preview) {
  return SendDMXWithSequenceOffset(universe, buffer, 0, priority, preview);
}


/*
 * Send some DMX data, allowing finer grained control of parameters.
 * @param universe the id of the universe to send
 * @param buffer the DMX data
 * @param cid the cid to send from
 * @param sequence_offset used to twiddle the sequence numbers, this doesn't
 * increment the sequence counter.
 * @param priority the priority to use
 * @param preview set to true to turn on the preview bit
 * @return true if it was sent successfully, false otherwise
 */
bool E131Node::SendDMXWithSequenceOffset(uint16_t universe,
                                         const ola::DmxBuffer &buffer,
                                         int8_t sequence_offset,
                                         uint8_t priority,
                                         bool preview) {
  map<unsigned int, tx_universe>::iterator iter =
      m_tx_universes.find(universe);
  tx_universe *settings;

  if (iter == m_tx_universes.end())
    settings = SetupOutgoingSettings(universe);
  else
    settings = &iter->second;

  const uint8_t *dmp_data;
  unsigned int dmp_data_length;

  if (m_use_rev2) {
    dmp_data = buffer.GetRaw();
    dmp_data_length = buffer.Size();
  } else {
    unsigned int data_size = DMX_UNIVERSE_SIZE;
    buffer.Get(m_send_buffer + 1, &data_size);
    dmp_data = m_send_buffer;
    dmp_data_length = data_size + 1;
  }

  TwoByteRangeDMPAddress range_addr(0, 1, (uint16_t) dmp_data_length);
  DMPAddressData<TwoByteRangeDMPAddress> range_chunk(&range_addr,
                                                     dmp_data,
                                                     dmp_data_length);
  vector<DMPAddressData<TwoByteRangeDMPAddress> > ranged_chunks;
  ranged_chunks.push_back(range_chunk);
  const DMPPDU *pdu = NewRangeDMPSetProperty<uint16_t>(true,
                                                       false,
                                                       ranged_chunks);

  E131Header header(settings->source,
                    priority,
                    static_cast<uint8_t>(settings->sequence + sequence_offset),
                    universe,
                    preview,  // preview
                    false,  // terminated
                    m_use_rev2);

  bool result = m_e131_sender.SendDMP(header, pdu);
  if (result && !sequence_offset)
    settings->sequence++;
  delete pdu;
  return result;
}


/*
 * Signal termination of this stream for a universe.
 * @param universe the id of the universe to send
 * @param priority the priority to use, this doesn't actually make a
 * difference.
 */
bool E131Node::StreamTerminated(uint16_t universe,
                                const ola::DmxBuffer &buffer,
                                uint8_t priority) {
  map<unsigned int, tx_universe>::iterator iter =
      m_tx_universes.find(universe);

  string source_name;
  uint8_t sequence_number;

  if (iter == m_tx_universes.end()) {
    source_name = "";
    sequence_number = 0;
  } else {
    source_name = iter->second.source;
    sequence_number = iter->second.sequence;
  }

  unsigned int data_size = DMX_UNIVERSE_SIZE;
  buffer.Get(m_send_buffer + 1, &data_size);

  TwoByteRangeDMPAddress range_addr(0, 1, (uint16_t) data_size);
  DMPAddressData<TwoByteRangeDMPAddress> range_chunk(&range_addr,
                                                     m_send_buffer,
                                                     data_size + 1);
  vector<DMPAddressData<TwoByteRangeDMPAddress> > ranged_chunks;
  ranged_chunks.push_back(range_chunk);
  const DMPPDU *pdu = NewRangeDMPSetProperty<uint16_t>(true,
                                                       false,
                                                       ranged_chunks);

  E131Header header(source_name,
                    priority,
                    sequence_number,
                    universe,
                    false,  // preview
                    true,  // terminated
                    false);

  bool result = m_e131_sender.SendDMP(header, pdu);
  // only update if we were previously tracking this universe
  if (result && iter != m_tx_universes.end())
    iter->second.sequence++;
  delete pdu;
  return result;
}


/*
 * Set the closure to be called when we receive data for this universe.
 * @param universe the universe to register the handler for
 * @param handler the Callback0 to call when there is data for this universe.
 * Ownership of the closure is transferred to the node.
 */
bool E131Node::SetHandler(unsigned int universe,
                          DmxBuffer *buffer,
                          uint8_t *priority,
                          Callback0<void> *closure) {
  IPV4Address addr;
  if (!m_e131_sender.UniverseIP(universe, &addr)) {
    OLA_WARN << "Unable to determine multicast group for universe " <<
      universe;
    return false;
  }

  if (!m_socket.JoinMulticast(m_interface.ip_address, addr)) {
    OLA_WARN << "Failed to join multicast group " << addr;
    return false;
  }

  return m_dmp_inflator.SetHandler(universe, buffer, priority, closure);
}


/*
 * Remove the handler for this universe
 * @param universe the universe handler to remove
 * @param true if removed, false if it didn't exist
 */
bool E131Node::RemoveHandler(unsigned int universe) {
  IPV4Address addr;
  if (!m_e131_sender.UniverseIP(universe, &addr)) {
    OLA_WARN << "Unable to determine multicast group for universe " <<
      universe;
    return false;
  }

  if (!m_socket.LeaveMulticast(m_interface.ip_address, addr)) {
    OLA_WARN << "Failed to leave multicast group " << addr;
    return false;
  }

  return m_dmp_inflator.RemoveHandler(universe);
}


/*
 * Create a settings entry for an outgoing universe
 */
E131Node::tx_universe *E131Node::SetupOutgoingSettings(unsigned int universe) {
  tx_universe settings;
  std::stringstream str;
  str << "Universe " << universe;
  settings.source = str.str();
  settings.sequence = 0;
  map<unsigned int, tx_universe>::iterator iter =
      m_tx_universes.insert(
          std::pair<unsigned int, tx_universe>(universe, settings)).first;
  return &iter->second;
}
}  // namespace e131
}  // namespace plugin
}  // namespace ola
