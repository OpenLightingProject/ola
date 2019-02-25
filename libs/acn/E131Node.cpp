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
 * E131Node.cpp
 * A E1.31 node
 * Copyright (C) 2005 Simon Newton
 */

#include <string.h>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/network/InterfacePicker.h"
#include "ola/stl/STLUtils.h"
#include "libs/acn/E131Node.h"

namespace ola {
namespace acn {

using ola::Callback0;
using ola::DmxBuffer;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::HostToNetwork;
using std::auto_ptr;
using std::map;
using std::string;
using std::set;
using std::vector;

class TrackedSource {
 public:
  TrackedSource()
      : clean_counter(0),
        current_sequence_number(0),
        total_pages(0) {
  }

  IPV4Address ip_address;
  string source_name;
  set<uint16_t> universes;

  uint8_t clean_counter;

  void NewPage(uint8_t page_number, uint8_t last_page,
               uint32_t sequence_number,
               const vector<uint16_t> &universes);

 private:
  uint32_t current_sequence_number;
  uint16_t total_pages;
  set<uint8_t> received_pages;
  set<uint16_t> new_universes;
};

void TrackedSource::NewPage(uint8_t page_number, uint8_t last_page,
                            uint32_t sequence_number,
                            const vector<uint16_t> &rx_universes) {
  clean_counter = 0;

  // This is broken because we don't actually get a sequence number in the
  // packet yet.
  // TODO(simon): Get the draft updated and fix this.
  if (sequence_number != current_sequence_number ||
      total_pages != last_page) {
    current_sequence_number = sequence_number;
    total_pages = last_page;
    received_pages.clear();
    new_universes.clear();
  }

  received_pages.insert(page_number);
  std::copy(rx_universes.begin(), rx_universes.end(),
            std::inserter(new_universes, new_universes.end()));

  uint8_t expected_page = 0;
  set<uint8_t>::const_iterator iter = received_pages.begin();
  for (; iter != received_pages.end(); ++iter) {
    if (*iter != expected_page)
      return;

    expected_page++;
  }

  if (expected_page == total_pages + 1) {
    universes = new_universes;
    received_pages.clear();
    new_universes.clear();
    total_pages = 0;
  }
}

E131Node::E131Node(ola::thread::SchedulerInterface *ss,
                   const string &ip_address,
                   const Options &options,
                   const ola::acn::CID &cid)
    : m_ss(ss),
      m_options(options),
      m_preferred_ip(ip_address),
      m_cid(cid),
      m_root_sender(m_cid),
      m_e131_sender(&m_socket, &m_root_sender),
      m_dmp_inflator(options.ignore_preview),
      m_discovery_inflator(NewCallback(this, &E131Node::NewDiscoveryPage)),
      m_incoming_udp_transport(&m_socket, &m_root_inflator),
      m_send_buffer(NULL),
      m_discovery_timeout(ola::thread::INVALID_TIMEOUT) {


  if (!m_options.use_rev2) {
    // Allocate a buffer for the dmx data + start code
    m_send_buffer = new uint8_t[DMX_UNIVERSE_SIZE + 1];
    m_send_buffer[0] = 0;  // start code is 0
  }

  // setup all the inflators
  m_root_inflator.AddInflator(&m_e131_inflator);
  m_root_inflator.AddInflator(&m_e131_rev2_inflator);
  m_e131_inflator.AddInflator(&m_dmp_inflator);
  m_e131_inflator.AddInflator(&m_discovery_inflator);
  m_e131_rev2_inflator.AddInflator(&m_dmp_inflator);
}


/*
 * Cleanup
 */
E131Node::~E131Node() {
  // remove handlers for all universes. This also leaves the multicast groups.
  vector<uint16_t> universes;
  m_dmp_inflator.RegisteredUniverses(&universes);
  vector<uint16_t>::const_iterator iter = universes.begin();
  for (; iter != universes.end(); ++iter) {
    RemoveHandler(*iter);
  }

  Stop();
  if (m_send_buffer)
    delete[] m_send_buffer;

  STLDeleteValues(&m_discovered_sources);
}


bool E131Node::Start() {
  auto_ptr<ola::network::InterfacePicker> picker(
    ola::network::InterfacePicker::NewPicker());
  if (!picker->ChooseInterface(&m_interface, m_preferred_ip)) {
    OLA_INFO << "Failed to find an interface";
    return false;
  }

  if (!m_socket.Init()) {
    return false;
  }

  if (!m_socket.Bind(IPV4SocketAddress(IPV4Address::WildCard(),
                                       m_options.port))) {
    return false;
  }

  if (!m_socket.EnableBroadcast()) {
    return false;
  }

  m_socket.SetTos(m_options.dscp);
  m_socket.SetMulticastInterface(m_interface.ip_address);

  m_socket.SetOnData(NewCallback(&m_incoming_udp_transport,
                                 &IncomingUDPTransport::Receive));

  if (m_options.enable_draft_discovery) {
    IPV4Address addr;
    m_e131_sender.UniverseIP(DISCOVERY_UNIVERSE_ID, &addr);

    if (!m_socket.JoinMulticast(m_interface.ip_address, addr)) {
      OLA_WARN << "Failed to join multicast group " << addr;
    }

    m_discovery_timeout = m_ss->RegisterRepeatingTimeout(
        UNIVERSE_DISCOVERY_INTERVAL,
        ola::NewCallback(this, &E131Node::PerformDiscoveryHousekeeping));
  }
  return true;
}

bool E131Node::Stop() {
  m_ss->RemoveTimeout(m_discovery_timeout);
  m_discovery_timeout = ola::thread::INVALID_TIMEOUT;
  return true;
}

bool E131Node::SetSourceName(uint16_t universe, const string &source) {
  ActiveTxUniverses::iterator iter = m_tx_universes.find(universe);

  if (iter == m_tx_universes.end()) {
    tx_universe *settings = SetupOutgoingSettings(universe);
    settings->source = source;
  } else {
    iter->second.source = source;
  }
  return true;
}

bool E131Node::StartStream(uint16_t universe) {
  ActiveTxUniverses::iterator iter = m_tx_universes.find(universe);

  if (iter == m_tx_universes.end()) {
    SetupOutgoingSettings(universe);
  } else {
    OLA_WARN << "Trying to StartStream on universe " << universe << " which "
             << "is already started";
    return false;
  }
  return true;
}

bool E131Node::TerminateStream(uint16_t universe, uint8_t priority) {
  // The standard says to send this 3 times
  for (unsigned int i = 0; i < 3; i++) {
    SendStreamTerminated(universe, DmxBuffer(), priority);
  }
  STLRemove(&m_tx_universes, universe);
  return true;
}

bool E131Node::SendDMX(uint16_t universe,
                       const ola::DmxBuffer &buffer,
                       uint8_t priority,
                       bool preview) {
  return SendDMXWithSequenceOffset(universe, buffer, 0, priority, preview);
}


bool E131Node::SendDMXWithSequenceOffset(uint16_t universe,
                                         const ola::DmxBuffer &buffer,
                                         int8_t sequence_offset,
                                         uint8_t priority,
                                         bool preview) {
  ActiveTxUniverses::iterator iter = m_tx_universes.find(universe);
  tx_universe *settings;

  if (iter == m_tx_universes.end()) {
    settings = SetupOutgoingSettings(universe);
  } else {
    settings = &iter->second;
  }

  const uint8_t *dmp_data;
  unsigned int dmp_data_length;

  if (m_options.use_rev2) {
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
                    m_options.use_rev2);

  bool result = m_e131_sender.SendDMP(header, pdu);
  if (result && !sequence_offset)
    settings->sequence++;
  delete pdu;
  return result;
}

bool E131Node::SendStreamTerminated(uint16_t universe,
                                    const ola::DmxBuffer &buffer,
                                    uint8_t priority) {
  ActiveTxUniverses::iterator iter = m_tx_universes.find(universe);

  string source_name;
  uint8_t sequence_number;

  if (iter == m_tx_universes.end()) {
    source_name = m_options.source_name;
    sequence_number = 0;
  } else {
    source_name = iter->second.source;
    sequence_number = iter->second.sequence;
  }

  unsigned int data_size = DMX_UNIVERSE_SIZE;
  buffer.Get(m_send_buffer + 1, &data_size);
  data_size++;

  TwoByteRangeDMPAddress range_addr(0, 1, (uint16_t) data_size);
  DMPAddressData<TwoByteRangeDMPAddress> range_chunk(
      &range_addr, m_send_buffer, data_size);
  vector<DMPAddressData<TwoByteRangeDMPAddress> > ranged_chunks;
  ranged_chunks.push_back(range_chunk);
  const DMPPDU *pdu = NewRangeDMPSetProperty<uint16_t>(
      true, false, ranged_chunks);

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

bool E131Node::SetHandler(uint16_t universe,
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

bool E131Node::RemoveHandler(uint16_t universe) {
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


void E131Node::GetKnownControllers(std::vector<KnownController> *controllers) {
  TrackedSources::const_iterator iter = m_discovered_sources.begin();
  for (; iter != m_discovered_sources.end(); ++iter) {
    controllers->push_back(KnownController());
    KnownController &controller = controllers->back();

    controller.cid = iter->first;
    controller.ip_address = iter->second->ip_address;
    controller.source_name = iter->second->source_name;
    controller.universes = iter->second->universes;
  }
}

/*
 * Create a settings entry for an outgoing universe
 */
E131Node::tx_universe *E131Node::SetupOutgoingSettings(uint16_t universe) {
  tx_universe settings;
  settings.source = m_options.source_name;
  settings.sequence = 0;
  ActiveTxUniverses::iterator iter =
      m_tx_universes.insert(std::make_pair(universe, settings)).first;
  return &iter->second;
}


bool E131Node::PerformDiscoveryHousekeeping() {
  // Send the Universe Discovery packets.
  vector<uint16_t> universes;
  STLKeys(m_tx_universes, &universes);

  uint8_t last_page = static_cast<uint8_t>(
    universes.size() / DISCOVERY_PAGE_SIZE);
  uint32_t sequence_number = 0;
  for (uint8_t i = 0; i <= last_page; i++) {
    SendDiscoveryPage(universes, i, last_page, sequence_number);
  }

  // Delete any sources that we haven't heard from in 2 x
  // UNIVERSE_DISCOVERY_INTERVAL.
  TrackedSources::iterator iter = m_discovered_sources.begin();
  while (iter != m_discovered_sources.end()) {
    if (iter->second->clean_counter >= 2) {
      delete iter->second;
      OLA_INFO << "Removing " << iter->first.ToString() << " due to inactivity";
      m_discovered_sources.erase(iter++);
    } else {
      iter->second->clean_counter++;
      iter++;
    }
  }

  return true;
}

void E131Node::NewDiscoveryPage(
    const HeaderSet &headers,
    const E131DiscoveryInflator::DiscoveryPage &page) {
  if (!m_options.enable_draft_discovery) {
    return;
  }

  TrackedSources::iterator iter = STLLookupOrInsertNull(
      &m_discovered_sources,
      headers.GetRootHeader().GetCid());
  if (!iter->second) {
    iter->second = new TrackedSource();
    iter->second->ip_address = headers.GetTransportHeader().Source().Host();
    iter->second->source_name = headers.GetE131Header().Source();
  }

  TrackedSource *source = iter->second;
  if (source->ip_address != headers.GetTransportHeader().Source().Host()) {
    OLA_INFO << "CID " << headers.GetRootHeader().GetCid().ToString()
             << " changed from " << source->ip_address << " to "
             << headers.GetTransportHeader().Source().Host();
    source->ip_address = headers.GetTransportHeader().Source().Host();
  }
  source->source_name = headers.GetE131Header().Source();
  source->NewPage(page.page_number, page.last_page, page.page_sequence,
                  page.universes);
}

void E131Node::SendDiscoveryPage(const std::vector<uint16_t> &universes,
                                 uint8_t this_page,
                                 uint8_t last_page,
                                 OLA_UNUSED uint32_t sequence_number) {
  uint16_t in_this_page = static_cast<uint16_t>(this_page == last_page ?
      universes.size() % DISCOVERY_PAGE_SIZE : DISCOVERY_PAGE_SIZE);

  uint16_t *page_data = new uint16_t[in_this_page + 1];
  page_data[0] = HostToNetwork(
      static_cast<uint16_t>(this_page << 8 | last_page));

  for (unsigned int i = 0; i < in_this_page; i++) {
    page_data[i + 1] = HostToNetwork(
        universes[this_page * DISCOVERY_PAGE_SIZE + i]);
  }

  E131Header header(m_options.source_name, 0, 0, DISCOVERY_UNIVERSE_ID);
  m_e131_sender.SendDiscoveryData(
      header, reinterpret_cast<uint8_t*>(page_data), (in_this_page + 1) * 2);
  delete[] page_data;
}
}  // namespace acn
}  // namespace ola
