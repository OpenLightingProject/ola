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
 * E131Node.cpp
 * A E131 node
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string.h>
#include <map>
#include <string>
#include <vector>
#include "ola/Logging.h"
#include "plugins/e131/e131/E131Node.h"

namespace ola {
namespace plugin {
namespace e131 {

using std::string;
using ola::Closure;
using ola::DmxBuffer;


/*
 * Create a new node
 * @param ip_address the IP address to prefer to listen on
 * @param cid, the CID to use, if not provided we generate one
 * one.
 */
E131Node::E131Node(const string &ip_address, const CID &cid)
    : m_preferred_ip(ip_address),
      m_cid(cid),
      m_transport(),
      m_root_layer(&m_transport, m_cid),
      m_e131_layer(&m_root_layer),
      m_dmp_inflator(&m_e131_layer) {
}


/*
 * Cleanup
 */
E131Node::~E131Node() {
  Stop();
}


/*
 * Start this node
 */
bool E131Node::Start() {
  ola::network::InterfacePicker picker;
  ola::network::Interface interface;
  if (!picker.ChooseInterface(&interface, m_preferred_ip)) {
    OLA_INFO << "Failed to find an interface";
    return false;
  }

  if (!m_transport.Init(interface)) {
    return false;
  }

  m_e131_layer.SetInflator(&m_dmp_inflator);
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
 * Set the priority for a universe
 */
bool E131Node::SetSourcePriority(unsigned int universe, uint8_t priority) {
  map<unsigned int, tx_universe>::iterator iter =
      m_tx_universes.find(universe);

  if (iter == m_tx_universes.end()) {
    tx_universe *settings = SetupOutgoingSettings(universe);
    settings->priority = priority;
  } else {
    iter->second.priority = priority;
  }
  return true;
}


/*
 * Send some DMX data
 * @param universe the id of the universe to send
 * @param buffer the DMX data
 * @return true if it was send successfully, false otherwise
 */
bool E131Node::SendDMX(uint16_t universe,
                       const ola::DmxBuffer &buffer) {
  map<unsigned int, tx_universe>::iterator iter =
      m_tx_universes.find(universe);
  tx_universe *settings;

  if (iter == m_tx_universes.end())
    settings = SetupOutgoingSettings(universe);
  else
    settings = &iter->second;

  TwoByteRangeDMPAddress range_addr(0, 1, (uint16_t) buffer.Size());
  DMPAddressData<TwoByteRangeDMPAddress> range_chunk(&range_addr,
                                                     buffer.GetRaw(),
                                                     buffer.Size());
  vector<DMPAddressData<TwoByteRangeDMPAddress> > ranged_chunks;
  ranged_chunks.push_back(range_chunk);
  const DMPPDU *pdu = NewRangeDMPSetProperty<uint16_t>(true, false,
                                                       ranged_chunks);

  E131Header header(settings->source, settings->priority, settings->sequence,
                    universe);
  bool result = m_e131_layer.SendDMP(header, pdu);
  if (result)
    settings->sequence++;
  delete pdu;
  return result;
}


/*
 * Set the closure to be called when we receive data for this universe.
 * @param universe the universe to register the handler for
 * @param handler the Closure to call when there is data for this universe.
 * Ownership of the closure is transferred to the node.
 */
bool E131Node::SetHandler(unsigned int universe,
                          DmxBuffer *buffer,
                          Closure *closure) {
  return m_dmp_inflator.SetHandler(universe, buffer, closure);
}


/*
 * Remove the handler for this universe
 * @param universe the universe handler to remove
 * @param true if removed, false if it didn't exist
 */
bool E131Node::RemoveHandler(unsigned int universe) {
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
  settings.priority = DEFAULT_PRIORITY;
  settings.sequence = 0;
  map<unsigned int, tx_universe>::iterator iter =
      m_tx_universes.insert(
          std::pair<unsigned int, tx_universe>(universe, settings)).first;
  return &iter->second;
}
}  // e131
}  // plugin
}  // ola
