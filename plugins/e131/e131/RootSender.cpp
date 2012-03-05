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
 * RootSender.cpp
 * The RootSender class manages the sending of Root Layer PDUs.
 * Copyright (C) 2007 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include "ola/Logging.h"
#include "plugins/e131/e131/RootSender.h"
#include "plugins/e131/e131/Transport.h"

namespace ola {
namespace plugin {
namespace e131 {


/*
 * Create a new RootSender
 * @param cid The CID to send in the Root PDU.
 */
RootSender::RootSender(const CID &cid)
    : m_root_pdu(0) {
  m_root_pdu.Cid(cid);
}


/*
 * Encapsulate this PDU in a RootPDU and send it to the destination.
 * @param vector the vector to use at the root level
 * @param pdu the pdu to send.
 * @param transport the OutgoingTransport to use when sending the message.
 */
bool RootSender::SendPDU(unsigned int vector,
                         const PDU &pdu,
                         OutgoingTransport *transport) {
  m_working_block.Clear();
  m_working_block.AddPDU(&pdu);
  return SendPDUBlock(vector, m_working_block, transport);
}


/*
 * This is used to inject a packet from a different CID.
 * @param vector the vector to use at the root level
 * @param pdu the pdu to send.
 * @param cid the cid to send from
 * @param transport the OutgoingTransport to use when sending the message.
 */
bool RootSender::SendPDU(unsigned int vector,
                         const PDU &pdu,
                         const CID &cid,
                         OutgoingTransport *transport) {
  if (!transport)
    return false;

  PDUBlock<PDU> root_block, working_block;
  working_block.AddPDU(&pdu);
  RootPDU root_pdu(vector);
  root_pdu.Cid(cid);
  root_pdu.SetBlock(&working_block);
  root_block.AddPDU(&root_pdu);
  return transport->Send(root_block);
}


/*
 * Encapsulate this PDUBlock in a RootPDU and send it to the destination.
 * @param vector the vector to use at the root level
 * @param block the PDUBlock to send.
 * @param transport the OutgoingTransport to use when sending the message.
 */
bool RootSender::SendPDUBlock(unsigned int vector,
                              const PDUBlock<PDU> &block,
                              OutgoingTransport *transport) {
  if (!transport)
    return false;

  m_root_pdu.SetVector(vector);
  m_root_pdu.SetBlock(&block);
  m_root_block.Clear();
  m_root_block.AddPDU(&m_root_pdu);
  return transport->Send(m_root_block);
}
}  // e131
}  // plugin
}  // ola
