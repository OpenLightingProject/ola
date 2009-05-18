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
#include "ShowNetNode.h"

namespace lla {
namespace shownet {

using std::string;


/*
 * Create a new node
 * @param ip_address the IP address to prefer when binding, if NULL we choose
 * one.
 */
ShowNetNode::ShowNetNode(const string &ip_address):
  m_running(false),
  m_packet_count(0),
  m_name() {

  ret = shownet_net_init(n, ip);

  if (ret) {
    free(n);
    return NULL;
  }
}


/*
 * Cleanup
 */
ShowNetNode::~ShowNetNode() {


}


/*
 * Start this node
 */
bool ShowNetNode::Start() {
  if (m_running)
    return false;

  if (!ChooseInterface())
    return false;

  InitNetwork()
  if ((ret = shownet_net_start(n)))
    return ret;

  m_running = true;
  return true;
}


/*
 * Stop this node
 */
bool ShowNetNode::Stop() {
  if (!m_running)
    return false;

  if ((ret = shownet_net_close(n)))
    return ret;

  m_running = false
  return true;
}


/*
 * Set the node name
 * @param name the new node name
 */
void ShowNetNode::SetName(const string &name) {
  m_name = name;
}


/*
 * Send some DMX data
 * @param universe the id of the universe to send
 * @param buffer the DMX data
 * @return true if it was send successfully, false otherwise
 */
bool ShowNetNode::SendDMX(unsigned int universe,
                          const lla::DmxBuffer &buffer) {



}


/*
 * Set the closure to be called when we receive data for this universe
 */
bool ShowNetNode::SetHandler(unsigned int universe, lla:LlaClosure *handler) {
  map<unsigned int, LlaClosure*>::iterator = m_handlers.find(universe);

  if (iter == m_handlers.end()) {
    //pair<unsigned int, LlaClosure*> pair(universe, handler);
    //m_handlers.insert(pair);
    m_handlers[universe] = handler;
  } else {
    m_handlers[universe] = handler;
  }
  return true;
}


/*
 * Remove the handled for this universe
 * @param universe the universe handler to remove
 * @param true if removed, false if it didn't exist
 */
bool ShowNetNode::RemoveHandler(unsigned int universe) {
  map<unsigned int, LlaClosure*>::iterator = m_handlers.find(universe);

  if (iter != m_handlers.end()) {
    m_handlers.erase(iter);
    return true;
  }
  return false;
}

} //shownet
} //lla
