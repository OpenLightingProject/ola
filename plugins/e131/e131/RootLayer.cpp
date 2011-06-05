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
 * RootLayer.cpp
 * The RootLayer
 * Copyright (C) 2007 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include "ola/Logging.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/e131/e131/RootLayer.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::IPV4Address;
using ola::network::NetworkToHost;

/*
 * Create a new RootLayer
 * @param parser the parser to use
 * @param ip_address the desired IP address to use
 * @param loop set to true to loop multicast packets
 */
RootLayer::RootLayer(UDPTransport *transport, const CID &cid)
    : m_transport(transport),
      m_root_pdu(0) {
  m_root_pdu.Cid(cid);
  if (!m_transport) {
    OLA_WARN << "transport is null, this won't work";
    return;
  }
  m_transport->SetInflator(&m_root_inflator);
}


/*
 * Add an inflator to the root level
 */
bool RootLayer::AddInflator(BaseInflator *inflator) {
  return m_root_inflator.AddInflator(inflator);
}


/*
 * Encapsulate this PDU in a RootPDU and send it to the destination.
 * @param vector the vector to use at the root level
 * @param pdu the pdu to send.
 * @param destination the ipv4 address to send to
 * @param port the destination port to send to
 */
bool RootLayer::SendPDU(unsigned int vector,
                        const PDU &pdu,
                        const IPV4Address &destination,
                        uint16_t port) {
  m_working_block.Clear();
  m_working_block.AddPDU(&pdu);
  return SendPDUBlock(vector, m_working_block, destination, port);
}


/*
 * This is used to inject a packet from a different CID.
 * @param vector the vector to use at the root level
 * @param pdu the pdu to send.
 * @param cid the cid to send from
 * @param destination the ipv4 address to send to
 * @param port the destination port to send to
 */
bool RootLayer::SendPDU(unsigned int vector,
                        const PDU &pdu,
                        const CID &cid,
                        const IPV4Address &destination,
                        uint16_t port) {
  if (!m_transport)
    return false;

  PDUBlock<PDU> root_block, working_block;
  working_block.AddPDU(&pdu);
  RootPDU root_pdu(vector);
  root_pdu.Cid(cid);
  root_pdu.SetBlock(&working_block);
  root_block.AddPDU(&root_pdu);
  return m_transport->Send(root_block, destination, port);
}


/*
 * Encapsulate this PDUBlock in a RootPDU and send it to the destination.
 * @param vector the vector to use at the root level
 * @param block the PDUBlock to send.
 * @param destination the ipv4 address to send to
 * @param port the destination port to send to
 */
bool RootLayer::SendPDUBlock(unsigned int vector,
                             const PDUBlock<PDU> &block,
                             const IPV4Address &destination,
                             uint16_t port) {
  if (!m_transport)
    return false;

  m_root_pdu.SetVector(vector);
  m_root_pdu.SetBlock(&block);
  m_root_block.Clear();
  m_root_block.AddPDU(&m_root_pdu);
  return m_transport->Send(m_root_block, destination, port);
}


/*
 * Join a multicast group
 */
bool RootLayer::JoinMulticast(const IPV4Address &group) {
  if (m_transport)
    return m_transport->JoinMulticast(group);
  return false;
}


/*
 * Leave a multicast group
 */
bool RootLayer::LeaveMulticast(const IPV4Address &group) {
  if (m_transport)
    return m_transport->LeaveMulticast(group);
  return false;
}
}  // e131
}  // plugin
}  // ola
