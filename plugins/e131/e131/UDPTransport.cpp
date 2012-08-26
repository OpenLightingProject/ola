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
 * OutgoingUDPTransport.cpp
 * The OutgoingUDPTransport class
 * Copyright (C) 2007 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <string.h>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/e131/e131/BaseInflator.h"
#include "plugins/e131/e131/HeaderSet.h"
#include "plugins/e131/e131/UDPTransport.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::HostToNetwork;
using ola::network::IPV4Address;

/*
 * Send a block of PDU messages.
 * @param pdu_block the block of pdus to send
 */
bool OutgoingUDPTransport::Send(const PDUBlock<PDU> &pdu_block) {
  return m_impl->Send(pdu_block, m_destination, m_port);
}


/*
 * Send a block of PDU messages using UDP.
 * @param pdu_block the block of pdus to send
 * @param destination the ipv4 address to send to
 * @param port the destination port to send to
 */
bool OutgoingUDPTransportImpl::Send(const PDUBlock<PDU> &pdu_block,
                                    const IPV4Address &destination,
                                    uint16_t port) {
  unsigned int data_size;
  const uint8_t *data = m_packer->Pack(pdu_block, &data_size);

  if (!data)
    return false;

  return m_socket->SendTo(data, data_size, destination, port);
}



IncomingUDPTransport::IncomingUDPTransport(ola::network::UDPSocket *socket,
                                           BaseInflator *inflator)
    : m_socket(socket),
      m_inflator(inflator),
      m_recv_buffer(NULL) {
}


/*
 * Called when new data arrives.
 */
void IncomingUDPTransport::Receive() {
  if (!m_recv_buffer)
    m_recv_buffer = new uint8_t[PreamblePacker::MAX_DATAGRAM_SIZE];

  ssize_t size = PreamblePacker::MAX_DATAGRAM_SIZE;
  ola::network::IPV4Address src_address;
  uint16_t src_port;

  if (!m_socket->RecvFrom(m_recv_buffer, &size, src_address, src_port))
    return;

  unsigned int header_size = PreamblePacker::ACN_HEADER_SIZE;
  if (size < static_cast<ssize_t>(header_size)) {
    OLA_WARN << "short ACN frame, discarding";
    return;
  }

  if (memcmp(m_recv_buffer, PreamblePacker::ACN_HEADER, header_size)) {
    OLA_WARN << "ACN header is bad, discarding";
    return;
  }

  HeaderSet header_set;
  TransportHeader transport_header(src_address,
                                   src_port,
                                   TransportHeader::UDP);
  header_set.SetTransportHeader(transport_header);

  m_inflator->InflatePDUBlock(
      header_set,
      m_recv_buffer + header_size,
      static_cast<unsigned int>(size) - header_size);
  return;
}
}  // e131
}  // plugin
}  // ola
