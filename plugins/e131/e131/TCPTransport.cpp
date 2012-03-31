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
 * TCPTransport.cpp
 * The TCPTransport class
 * Copyright (C) 2012 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first

#include <ola/Logging.h>
#include "plugins/e131/e131/BaseInflator.h"
#include "plugins/e131/e131/HeaderSet.h"
#include "plugins/e131/e131/TCPTransport.h"

namespace ola {
namespace plugin {
namespace e131 {


/*
 * Send a block of PDU messages.
 * @param pdu_block the block of pdus to send
 */
bool TCPTransport::Send(const PDUBlock<PDU> &pdu_block) {
  unsigned int data_size;
  const uint8_t *data = m_packer->Pack(pdu_block, &data_size);

  if (!data)
    return false;

  // TODO(simon): rather than just attempting to send we should buffer here.
  return m_descriptor->Send(data, data_size);
}


IncomingTCPTransport::IncomingTCPTransport(BaseInflator *inflator)
    : m_inflator(inflator),
      m_recv_buffer(NULL) {
  m_acn_header[0] = PreamblePacker::PREAMBLE_SIZE >> 8;
  m_acn_header[1] = PreamblePacker::PREAMBLE_SIZE;
  m_acn_header[2] = PreamblePacker::POSTABLE_SIZE >> 8;
  m_acn_header[3] = PreamblePacker::POSTABLE_SIZE;
  memcpy(m_acn_header + PreamblePacker::PREAMBLE_OFFSET,
         PreamblePacker::ACN_PACKET_ID,
         PreamblePacker::DATA_OFFSET - PreamblePacker::PREAMBLE_OFFSET);
}


/*
 * Called when new data arrives.
 */
void IncomingTCPTransport::Receive(ola::network::TcpSocket *socket) {
  if (!m_recv_buffer)
    m_recv_buffer = new uint8_t[PreamblePacker::MAX_DATAGRAM_SIZE];

  unsigned int size = PreamblePacker::MAX_DATAGRAM_SIZE;
  if (0 != socket->Receive(m_recv_buffer,
                           PreamblePacker::MAX_DATAGRAM_SIZE,
                           size)) {
    OLA_WARN << "tcp rx failed";
    return;
  }

  if (size < (ssize_t) PreamblePacker::DATA_OFFSET) {
    OLA_WARN << "short ACN frame, discarding";
    return;
  }

  if (memcmp(m_recv_buffer, m_acn_header, PreamblePacker::DATA_OFFSET)) {
    OLA_WARN << "ACN header is bad, discarding";
    return;
  }

  HeaderSet header_set;
  uint16_t port;
  IPV4Address ip_address;
  socket->GetPeer(&ip_address, &port);
  TransportHeader transport_header(ip_address, port, TransportHeader::TCP);
  header_set.SetTransportHeader(transport_header);

  m_inflator->InflatePDUBlock(
      header_set,
      m_recv_buffer + PreamblePacker::DATA_OFFSET,
      static_cast<unsigned int>(size) - PreamblePacker::DATA_OFFSET);
  return;
}
}  // e131
}  // plugin
}  // ola
