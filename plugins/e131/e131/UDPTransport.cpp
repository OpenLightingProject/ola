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
 * UDPTransport.h
 * The UDPTransport class
 * Copyright (C) 2007 Simon Newton
 */

#include <string.h>
#include <ola/Closure.h>
#include <ola/Logging.h>
#include "UDPTransport.h"

namespace ola {
namespace e131 {

const string UDPTransport::ACN_PACKET_ID = "ASC-E1.17\0\0\0";

/*
 * Clean up
 */
UDPTransport::~UDPTransport() {
  m_socket.Close();
  if (m_send_buffer)
    delete[] m_send_buffer;
  if (m_recv_buffer)
    delete[] m_recv_buffer;
}


/*
 * Setup the UDP Transport
 */
bool UDPTransport::Init(const ola::network::Interface &interface) {
  if (!m_socket.Init())
    return false;

  if (!m_socket.Bind(m_port))
    return false;

  if (!m_socket.EnableBroadcast())
    return false;

  m_socket.SetOnData(NewClosure(this, &UDPTransport::Receive));

  if (!m_send_buffer) {
    m_send_buffer = new uint8_t[MAX_DATAGRAM_SIZE];
    memset(m_send_buffer, 0 , DATA_OFFSET);
    uint16_t *ptr = (uint16_t*) m_send_buffer;
    *ptr++ = htons(PREAMBLE_SIZE);
    *ptr = htons(POSTABLE_SIZE);
    strncpy((char*) (m_send_buffer + PREAMBLE_OFFSET),
            ACN_PACKET_ID.data(),
            ACN_PACKET_ID.size());
  }

  if (!m_recv_buffer)
    m_recv_buffer = new uint8_t[MAX_DATAGRAM_SIZE];

  m_interface = interface;
  return true;
}


/*
 * Send a block of PDU messages, this may send separate packets if the size of
 * the block is greater than the MAX_DATAGRAM_SIZE.
 * @param pdu_block the block of pdus to send
 */
bool UDPTransport::Send(const PDUBlock<PDU> &pdu_block,
                        const struct sockaddr_in &destination) {
  if (!m_send_buffer) {
    OLA_WARN << "Send called the transport hasn't been initialized";
    return false;
  }

  unsigned int size = MAX_DATAGRAM_SIZE - DATA_OFFSET;
  if (!pdu_block.Pack(m_send_buffer + DATA_OFFSET, size))
    return false;

  return m_socket.SendTo(m_send_buffer, DATA_OFFSET + size, destination);
}


/*
 * Called when new data arrives.
 */
int UDPTransport::Receive() {
  if (!m_recv_buffer) {
    OLA_WARN << "Receive called the transport hasn't been initialized";
    return 0;
  }

  ssize_t size = MAX_DATAGRAM_SIZE;
  if (!m_socket.RecvFrom(m_recv_buffer, size))
    return 0;

  if (size < (ssize_t) DATA_OFFSET) {
    OLA_WARN << "short ACN frame, discarding";
    return 0;
  }

  if (memcmp(m_recv_buffer, m_send_buffer, DATA_OFFSET)) {
    OLA_WARN << "ACN header is bad, discarding";
    return 0;
  }

  HeaderSet header_set;
  m_inflator->InflatePDUBlock(header_set,
                              m_recv_buffer + DATA_OFFSET,
                              size - DATA_OFFSET);
  return 0;
}


bool UDPTransport::JoinMulticast(const struct in_addr &group) {
  return m_socket.JoinMulticast(m_interface.ip_address, group);
}


bool UDPTransport::LeaveMulticast(const struct in_addr &group) {
  return m_socket.JoinMulticast(m_interface.ip_address, group);
}

} // e131
} // ola
