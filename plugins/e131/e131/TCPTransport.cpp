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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * TCPTransport.cpp
 * The classes for transporting ACN over TCP.
 * Copyright (C) 2012 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first

#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/network/SocketAddress.h>
#include <algorithm>
#include <iostream>
#include "plugins/e131/e131/BaseInflator.h"
#include "plugins/e131/e131/HeaderSet.h"
#include "plugins/e131/e131/TCPTransport.h"

namespace ola {
namespace plugin {
namespace e131 {

const uint8_t ACN_HEADER[] = {
  0x00, 0x14,  // preamble size
  0x00, 0x00,  // post amble size
  0x41, 0x53, 0x43, 0x2d,
  0x45, 0x31, 0x2e, 0x31,
  0x37, 0x00, 0x00, 0x00
  // For TCP, the next 4 bytes are the block size
};

const unsigned int ACN_HEADER_SIZE = sizeof(ACN_HEADER);

// TODO(simon): tune this once we have an idea of what the sizes will be
const unsigned int IncommingStreamTransport::INITIAL_SIZE = 500;


/**
 * Create a new IncommingStreamTransport.
 * @param inflator the inflator to call for each PDU
 * @param descriptor the descriptor to read from
 * @param ip_address the IP to use in the transport header
 * @param port the port to use in the transport header
 */
IncommingStreamTransport::IncommingStreamTransport(
    BaseInflator *inflator,
    ola::io::ConnectedDescriptor *descriptor,
    const ola::network::IPV4SocketAddress &source)
    : m_transport_header(source, TransportHeader::TCP),
      m_inflator(inflator),
      m_descriptor(descriptor),
      m_buffer_start(NULL),
      m_buffer_end(NULL),
      m_data_end(NULL),
      m_block_size(0),
      m_consumed_block_size(0),
      m_stream_valid(true),
      m_pdu_length_size(TWO_BYTES) {
  EnterWaitingForPreamble();
}


/**
 * Clean up
 */
IncommingStreamTransport::~IncommingStreamTransport() {
  if (m_buffer_start)
    delete[] m_buffer_start;
}


/**
 * Read from this stream, looking for ACN messages.
 * @returns false if the stream is no longer consistent. At this point the
 * caller should close the descriptor since the data is no longer valid.
 */
bool IncommingStreamTransport::Receive() {
  while (true) {
    OLA_DEBUG << "start read, outstanding bytes is " << m_outstanding_data;
    // Read as much as we need
    ReadRequiredData();

    OLA_DEBUG << "done read, bytes outstanding is " << m_outstanding_data;

    // if we still don't have enough, return
    if (m_stream_valid == false || m_outstanding_data)
      return m_stream_valid;

    OLA_DEBUG << "state is " << m_state;

    switch (m_state) {
      case WAITING_FOR_PREAMBLE:
        HandlePreamble();
        break;
      case WAITING_FOR_PDU_FLAGS:
        HandlePDUFlags();
        break;
      case WAITING_FOR_PDU_LENGTH:
        HandlePDULength();
        break;
      case WAITING_FOR_PDU:
        HandlePDU();
        break;
    }
    if (!m_stream_valid)
      return false;
  }
}


/**
 * Handle the Preamble data.
 * @pre 20 bytes in the buffer
 */
void IncommingStreamTransport::HandlePreamble() {
  OLA_DEBUG << "in handle preamble, data len is " << DataLength();

  if (memcmp(m_buffer_start, ACN_HEADER, ACN_HEADER_SIZE) != 0) {
    ola::FormatData(&std::cout, m_buffer_start, ACN_HEADER_SIZE);
    ola::FormatData(&std::cout, ACN_HEADER, ACN_HEADER_SIZE);
    OLA_WARN << "bad ACN header";
    m_stream_valid = false;
    return;
  }

  // read the PDU block length
  memcpy(reinterpret_cast<void*>(&m_block_size),
         m_buffer_start + ACN_HEADER_SIZE,
         sizeof(m_block_size));
  m_block_size = ola::network::NetworkToHost(m_block_size);
  OLA_DEBUG << "pdu block size is " << m_block_size;

  if (m_block_size) {
    m_consumed_block_size = 0;
    EnterWaitingForPDU();
  } else {
    EnterWaitingForPreamble();
  }
}


/**
 * Handle the PDU Flag data, this allows us to figure out how many bytes we
 * need to read the length.
 * @pre 1 byte in the buffer
 */
void IncommingStreamTransport::HandlePDUFlags() {
  OLA_DEBUG << "Reading PDU flags, data size is " << DataLength();
  m_pdu_length_size = (*m_buffer_start  & BaseInflator::LFLAG_MASK) ?
    THREE_BYTES : TWO_BYTES;
  m_outstanding_data += static_cast<unsigned int>(m_pdu_length_size) - 1;
  OLA_DEBUG << "PDU length size is " << static_cast<int>(m_pdu_length_size) <<
    " bytes";
  m_state = WAITING_FOR_PDU_LENGTH;
}


/**
 * Handle the PDU Length data.
 * @pre 2 or 3 bytes of data in the buffer, depending on m_pdu_length_size
 */
void IncommingStreamTransport::HandlePDULength() {
  if (m_pdu_length_size == THREE_BYTES) {
    m_pdu_size = (
      m_buffer_start[2] +
      static_cast<unsigned int>(m_buffer_start[1] << 8) +
      static_cast<unsigned int>((m_buffer_start[0] & BaseInflator::LENGTH_MASK)
        << 16));
  } else {
    m_pdu_size = m_buffer_start[1] + static_cast<unsigned int>(
        (m_buffer_start[0] & BaseInflator::LENGTH_MASK) << 8);
  }
  OLA_DEBUG << "PDU size is " << m_pdu_size;

  if (m_pdu_size < static_cast<unsigned int>(m_pdu_length_size)) {
    OLA_WARN << "PDU length was set to " << m_pdu_size << " but " <<
      static_cast<unsigned int>(m_pdu_length_size) <<
      " bytes were used in the header";
    m_stream_valid = false;
    return;
  }

  m_outstanding_data += (
    m_pdu_size - static_cast<unsigned int>(m_pdu_length_size));
  OLA_DEBUG << "Processed length, now waiting on another " << m_outstanding_data
    << " bytes";
  m_state = WAITING_FOR_PDU;
}


/**
 * Handle a PDU
 * @pre m_pdu_size bytes in the buffer
 */
void IncommingStreamTransport::HandlePDU() {
  OLA_DEBUG << "Got PDU, data length is " << DataLength() << ", expected " <<
    m_pdu_size;

  if (DataLength() != m_pdu_size) {
    OLA_WARN << "PDU size doesn't match the available data";
    m_stream_valid = false;
    return;
  }

  HeaderSet header_set;
  header_set.SetTransportHeader(m_transport_header);

  unsigned int data_consumed = m_inflator->InflatePDUBlock(
      &header_set,
      m_buffer_start,
      m_pdu_size);
  OLA_DEBUG << "inflator consumed " << data_consumed << " bytes";

  if (m_pdu_size != data_consumed) {
    OLA_WARN << "PDU inflation size mismatch, " << m_pdu_size << " != "
    << data_consumed;
    m_stream_valid = false;
    return;
  }

  m_consumed_block_size += data_consumed;

  if (m_consumed_block_size == m_block_size) {
    // all PDUs in this block have been processed
    EnterWaitingForPreamble();
  } else {
    EnterWaitingForPDU();
  }
}


/**
 * Grow the rx buffer to the new size.
 */
void IncommingStreamTransport::IncreaseBufferSize(unsigned int new_size) {
  if (new_size <= BufferSize())
    return;

  new_size = std::max(new_size, INITIAL_SIZE);

  unsigned int data_length = DataLength();
  if (!m_buffer_start)
    data_length = 0;

  // allocate new buffer and copy the data over
  uint8_t *buffer = new uint8_t[new_size];
  if (m_buffer_start) {
    if (data_length > 0)
      // this moves the data to the start of the buffer if it wasn't already
      memcpy(buffer, m_buffer_start, data_length);
    delete[] m_buffer_start;
  }

  m_buffer_start = buffer;
  m_buffer_end = buffer + new_size;
  m_data_end = buffer + data_length;
}


/**
 * Read data until we reach the number of bytes we required or there is no more
 * data to be read
 */
void IncommingStreamTransport::ReadRequiredData() {
  if (m_outstanding_data == 0)
    return;

  if (m_outstanding_data > FreeSpace())
    IncreaseBufferSize(DataLength() + m_outstanding_data);

  unsigned int data_read;
  int ok = m_descriptor->Receive(m_data_end,
                                 m_outstanding_data,
                                 data_read);

  if (ok != 0)
    OLA_WARN << "tcp rx failed";
  OLA_DEBUG << "read " << data_read;
  m_data_end += data_read;
  m_outstanding_data -= data_read;
}


/**
 * Enter the wait-for-preamble state
 */
void IncommingStreamTransport::EnterWaitingForPreamble() {
  m_data_end = m_buffer_start;
  m_state = WAITING_FOR_PREAMBLE;
  m_outstanding_data = ACN_HEADER_SIZE + PDU_BLOCK_SIZE;
}


/**
 * Enter the wait-for-pdu state
 */
void IncommingStreamTransport::EnterWaitingForPDU() {
  m_state = WAITING_FOR_PDU_FLAGS;
  m_data_end = m_buffer_start;
  // we need 1 byte to read the flags
  m_outstanding_data = 1;
}


/**
 * Create a new IncomingTCPTransport
 */
IncomingTCPTransport::IncomingTCPTransport(BaseInflator *inflator,
                                           ola::network::TCPSocket *socket)
    : m_transport(NULL) {
  ola::network::GenericSocketAddress address = socket->GetPeer();
  if (address.Family() == AF_INET) {
    ola::network::IPV4SocketAddress v4_addr = address.V4Addr();
    m_transport.reset(
        new IncommingStreamTransport(inflator, socket, v4_addr));
  } else {
    OLA_WARN << "Invalid address for fd " << socket->ReadDescriptor();
  }
}
}  // namespace e131
}  // namespace plugin
}  // namespace ola
