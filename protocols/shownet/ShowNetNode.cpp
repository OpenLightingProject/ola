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
#include "RunLengthEncoder.h"

namespace lla {
namespace shownet {

using std::string;
using std::map;
using lla::network::UdpSocket;
using lla::LlaClosure;


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

  if (!m_interface_picker.ChooseInterface(m_interface, m_preferred_ip))
    return false;

  if (!InitNetwork())
    return false;
  //if ((ret = shownet_net_start(n)))

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

  shownet_packet_t p;
  int ret, len, enc_len;

  len = min(length, SHOWNET_DMX_LENGTH);

  memset(&p.data, 0x00, sizeof(p.data));

  // set dst addr and length
  p.to.s_addr = n->state.bcast_addr.s_addr;
  p.length = sizeof(shownet_data_t);

  // setup the fields in the datagram
  p.data.dmx.sigHi = 0x80;
  p.data.dmx.sigLo = 0x8f;

  // set ip
  memcpy(p.data.dmx.ip, &n->state.ip_addr.s_addr,4);

  p.data.dmx.netSlot[0] = (universe * 0x0200) + 0x01;
  p.data.dmx.netSlot[1] = 0;
  p.data.dmx.netSlot[2] = 0;
  p.data.dmx.netSlot[3] = 0;
  p.data.dmx.slotSize[0] = len;
  p.data.dmx.slotSize[1] = 0;
  p.data.dmx.slotSize[2] = 0;
  p.data.dmx.slotSize[3] = 0;

  enc_len = encode_dmx(data,len, p.data.dmx.data, SHOWNET_DMX_LENGTH);

  p.data.dmx.indexBlock[0] = 0x0b;
  p.data.dmx.indexBlock[1] = 0x0b + enc_len;
  p.data.dmx.indexBlock[2] = 0;
  p.data.dmx.indexBlock[3] = 0;
  p.data.dmx.indexBlock[4] = 0;

  p.data.dmx.packetCountHi = short_gethi(n->state.packet_count);
  p.data.dmx.packetCountLo = short_getlo(n->state.packet_count);
  // magic numbers - not sure what these do
  p.data.dmx.block[2] = 0x58;
  p.data.dmx.block[3] = 0x02;
  strncpy((char*) p.data.dmx.name, n->state.name, SHOWNET_NAME_LENGTH);

  ret = shownet_net_send(n, &p);

  if (!ret)
    n->state.packet_count++;

  return ret;
}


/*
 * Set the closure to be called when we receive data for this universe
 */
bool ShowNetNode::SetHandler(unsigned int universe, LlaClosure *handler) {
  map<unsigned int, LlaClosure*>::iterator iter = m_handlers.find(universe);

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
  map<unsigned int, LlaClosure*>::iterator iter = m_handlers.find(universe);

  if (iter != m_handlers.end()) {
    m_handlers.erase(iter);
    return true;
  }
  return false;
}


/*
 * Called when there is data on this socket
 */
int ShowNetNode::SocketReady(class ConnectedSocket *socket) {



}


/*
 * Setup the networking compoents.
 */
bool ShowNetNode::InitNetwork() {
  m_socket = new UdpSocket();

  if (!m_socket->Init(SHOWNET_PORT)) {
    delete m_socket;
    return false;
  }

  if (!m_socket->EnableBroadcast()) {
    delete m_socket;
    return false;
  }

  // add ourself as a listenr
  m_socket->SetListener(this);

  return true;
}

} //shownet
} //lla
