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
#include <algorithm>
#include "plugins/e131/e131/BaseInflator.h"
#include "plugins/e131/e131/HeaderSet.h"
#include "plugins/e131/e131/PreamblePacker.h"
#include "plugins/e131/e131/TCPTransport.h"

namespace ola {
namespace plugin {
namespace e131 {

const unsigned int IncommingStreamTransport::ACN_HEADER_SIZE =
  PreamblePacker::ACN_HEADER_SIZE;

/*
 * Send a block of PDU messages.
 * @param pdu_block the block of pdus to send
 */
bool OutgoingStreamTransport::Send(const PDUBlock<PDU> &pdu_block) {
  unsigned int data_size;
  const uint8_t *data = m_packer->Pack(pdu_block, &data_size);

  if (!data)
    return false;

  // TODO(simon): rather than just attempting to send we should buffer here.
  return m_descriptor->Send(data, data_size);
}



/**
 * Create a new IncommingStreamTransport
 */
IncommingStreamTransport::IncommingStreamTransport(
    BaseInflator *inflator,
    ola::network::ConnectedDescriptor *descriptor,
    const IPV4Address &ip_address,
    uint16_t port)
    : m_transport_header(ip_address, port, TransportHeader::TCP),
      m_inflator(inflator),
      m_descriptor(descriptor),
      m_buffer_start(NULL),
      m_buffer_end(NULL),
      m_data_start(NULL),
      m_data_end(NULL) {
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
 */
void IncommingStreamTransport::Receive() {
  if (!m_buffer_start)
    IncreaseBufferSize(INITIAL_SIZE);

  if (!m_buffer_start)
    // all we can do here is return and hope on the next iteration we have more
    // memory.
    return;

  while (true) {
    OLA_INFO << "-- start iteration";
    uint8_t *old_data_end = m_data_end;
    if (!ReadChunk())
      // error
      return;

    OLA_DEBUG << "read " << (m_data_end - old_data_end) << " bytes";
    if (m_data_end == old_data_end) {
      // no further data
      // maybe realign the data here, it's cheaper to do it now than once we've
      // read more
      OLA_DEBUG << "no more data, realigning & returning";
      RealignBuffer();
      return;
    }

    OLA_INFO << (unsigned int*) m_data_start;
    OLA_INFO << (unsigned int*) m_data_end;
    OLA_DEBUG << "have " << DataLength() << ", offset at " <<
      (m_data_start - m_buffer_start);

    while (true) {
      // process as many ACN frames as we can before we exhaust all the data
      LookAheadForHeader();

      unsigned int data_length = DataLength();
      if (data_length == 0) {
        // all data was consumed, reset the buffer for the next read
        m_data_start = m_buffer_start;
        m_data_end = m_buffer_start - 1;
        OLA_DEBUG << "no header found in data, next iteration";
        break;
      }

      uint16_t message_length;
      if (data_length <= ACN_HEADER_SIZE + sizeof(message_length)) {
        // realign at this point
        // not enough data yet, try to read some more
        OLA_INFO << "possible header found @ " << (unsigned int*) m_data_start
          << " but not enough data, realigning";
        RealignBuffer();
        break;
      }

      // read length
      message_length = ((m_data_start[ACN_HEADER_SIZE] & 0x0F) << 8) +
                       m_data_start[ACN_HEADER_SIZE + 1];
      OLA_INFO << "header ok, message length is " << message_length;

      unsigned int packet_data_in_buffer = data_length - ACN_HEADER_SIZE;
      OLA_DEBUG << "packet data in buffer: " << packet_data_in_buffer <<
        ", contig space " << ContiguousSpace() << ", total free " <<
        AvailableBufferSpace();

      if (message_length <= packet_data_in_buffer) {
        // we have all the data we're looking for
        OLA_DEBUG << "got all our data, time to inflate";

        HeaderSet header_set;
        header_set.SetTransportHeader(m_transport_header);
        OLA_DEBUG << "inflating";
        unsigned int data_consumed = m_inflator->InflatePDUBlock(
            header_set,
            &m_data_start[ACN_HEADER_SIZE],
            message_length);
        OLA_DEBUG << "inflator consumed " << data_consumed << "bytes";
        m_data_start += data_consumed;
        // this frame is done, attempt to process the next one
        continue;
      }

      // not enough data, check that we have enough space to read it in
      if (message_length > packet_data_in_buffer + ContiguousSpace()) {
        // not enough space remaining at end of buffer
        if (message_length > packet_data_in_buffer + AvailableBufferSpace()) {
          // realigning won't help, we need to grow the buffer
          OLA_DEBUG << "growing buffer to " << message_length + ACN_HEADER_SIZE;
          // The max size for 2^12 is 4k which seems reasonable.
          IncreaseBufferSize(message_length + ACN_HEADER_SIZE);
        } else {
          OLA_DEBUG << "realigning buffer";
          RealignBuffer();
        }
        break;
      }
    }
  }
}


/**
 * Grow the rx buffer to the new size.
 */
void IncommingStreamTransport::IncreaseBufferSize(unsigned int new_size) {
  if (new_size <= BufferSize())
    return;

  unsigned int data_length = DataLength();
  if (!m_buffer_start)
    data_length = 0;

  // allocate new buffer and copy the data over
  uint8_t *buffer = new uint8_t[new_size];
  OLA_INFO << "new buffer at 0x" << (unsigned int*) buffer <<
    ", data length was " << data_length;
  if (m_buffer_start) {
    if (data_length > 0)
      // this moves the data to the start of the buffer if it wasn't already
      memcpy(buffer, m_data_start, data_length);
    delete[] m_buffer_start;
  }

  m_buffer_start = buffer;
  m_buffer_end = buffer + new_size;
  m_data_start = buffer;
  m_data_end = buffer + data_length;
}


/**
 * Read data until we fill up our buffer or no data remains
 * @returns false if there was an error, true otherwise
 */
bool IncommingStreamTransport::ReadChunk() {
  unsigned int data_read;
  OLA_INFO << "contig space is " << ContiguousSpace();
  int ok = m_descriptor->Receive(
      m_data_end,
      ContiguousSpace(),
      data_read);
  if (ok != 0)
    OLA_WARN << "tcp rx failed";
  OLA_DEBUG << "actually read " << data_read;
  m_data_end += data_read;
  OLA_INFO << "m_data_end is now " << (unsigned int*) m_data_end;
  return ok == 0;
}


/**
 * Look through the data to locate the ACN header.
 * This method increments m_data_start until all data is searched, or we find a
 * potential header match.
 */
void IncommingStreamTransport::LookAheadForHeader() {
  // Valid ACN messages start with PreamblePacker::ACN_HEADER
  while (true) {
    while (*m_data_start && m_data_start <= m_data_end)
      m_data_start++;

    if (m_data_start > m_data_end)
      // no data left
      return;

    unsigned int cmp_length = std::min(DataLength(), ACN_HEADER_SIZE);

    if (memcmp(m_data_start, PreamblePacker::ACN_HEADER, cmp_length) == 0)
      // we have a possible start-of-frame, return
      return;

    // otherwise move on to the next byte
    m_data_start++;
  }
}


/**
 * Re-align the buffer so the data starts from the beginning
 */
void IncommingStreamTransport::RealignBuffer() {
  unsigned int data_length = DataLength();

  // allocate new buffer and copy the data over
  if (data_length > 0 && m_data_start != m_buffer_start)
    // this moves the data to the start of the buffer if it wasn't already
    memcpy(m_buffer_start, m_data_start, data_length);

  m_data_start = m_buffer_start;
  m_data_end = m_data_start + data_length;
}


/**
 * Create a new IncomingTCPTransport
 */
IncomingTCPTransport::IncomingTCPTransport(BaseInflator *inflator,
                                           ola::network::TcpSocket *socket):
  m_transport(NULL) {
  uint16_t port;
  IPV4Address ip_address;
  socket->GetPeer(&ip_address, &port);
  m_transport.reset(
      new IncommingStreamTransport(inflator, socket, ip_address, port));
}
}  // e131
}  // plugin
}  // ola
