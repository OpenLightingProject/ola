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
#include <ola/Logging.h>
#include "E131Node.h"

namespace ola {
namespace e131 {

using std::string;
using std::map;
using ola::Closure;


/*
 * Create a new node
 * @param ip_address the IP address to prefer to listen on
 * @param cid, the CID to use, if not provided we generate one
 * one.
 */
E131Node::E131Node(const string &ip_address, const CID &cid):
  m_preferred_ip(ip_address),
  m_cid(cid),
  m_transport(),
  m_root_layer(&m_transport, m_cid),
  m_e131_layer(&m_root_layer) {

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
  if (!picker.ChooseInterface(interface, m_preferred_ip)) {
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
  std::map<unsigned int, universe_handler>::iterator iter;
  for (iter = m_handlers.begin(); iter != m_handlers.end(); ++iter) {
    delete iter->second.closure;
    m_e131_layer.LeaveUniverse(iter->first);
  }
  m_handlers.clear();
  return true;
}


/*
 * Send some DMX data
 * @param universe the id of the universe to send
 * @param buffer the DMX data
 * @return true if it was send successfully, false otherwise
 */
bool E131Node::SendDMX(unsigned int universe,
                       const ola::DmxBuffer &buffer) {

  /*
  if (!m_running)
    return false;

  if (universe >= E131_MAX_UNIVERSES) {
    OLA_WARN << "Universe index out of bounds, should be between 0 and" <<
                  E131_MAX_UNIVERSES << "), was " << universe;
    return false;
  }


  if (bytes_sent != size) {
    OLA_WARN << "Only sent " << bytes_sent << " of " << size;
    return false;
  }

  m_packet_count++;

  E131Header header;
  return m_e131_layer->SendDmp(header, e131_pdu);
  */
  return true;
}


/*
 * Get the DMX data for this universe.
 */
DmxBuffer E131Node::GetDMX(unsigned int universe) {
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
bool E131Node::SetHandler(unsigned int universe, Closure *closure) {
  if (!closure)
    return false;

  map<unsigned int, universe_handler>::iterator iter =
    m_handlers.find(universe);

  if (iter == m_handlers.end()) {
    universe_handler handler;
    handler.closure = closure;
    handler.buffer.Blackout();
    m_handlers[universe] = handler;
    m_e131_layer.JoinUniverse(universe);
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
bool E131Node::RemoveHandler(unsigned int universe) {
  map<unsigned int, universe_handler>::iterator iter =
    m_handlers.find(universe);

  if (iter != m_handlers.end()) {
    Closure *old_closure = iter->second.closure;
    m_handlers.erase(iter);
    delete old_closure;
    m_e131_layer.LeaveUniverse(universe);
    return true;
  }
  return false;
}



} //e131
} //ola
