/*
#include <ola/Logging.h>
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
 * TCPTransport.h
 * Interface for the TCPTransport class
 * Copyright (C) 2012 Simon Newton
 */

#ifndef PLUGINS_E131_E131_TCPTRANSPORT_H_
#define PLUGINS_E131_E131_TCPTRANSPORT_H_

#include <memory>
#include "ola/network/Socket.h"
#include "plugins/e131/e131/PDU.h"
#include "plugins/e131/e131/PreamblePacker.h"
#include "plugins/e131/e131/Transport.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::IPV4Address;

/*
 * Transport ACN messages over a stream socket. This is named TCPSocket, but we
 * can use any ConnectedDescriptor.
 */
class TCPTransport: public OutgoingTransport {
  public:
    TCPTransport(ola::network::ConnectedDescriptor *descriptor,
                 PreamblePacker *packer = NULL)
        : m_descriptor(descriptor),
          m_packer(packer),
          m_free_packer(false) {
      if (!m_packer) {
        m_packer = new PreamblePacker();
        m_free_packer = true;
      }
    }
    ~TCPTransport() {
      if (m_free_packer)
        delete m_packer;
    }

    bool Send(const PDUBlock<PDU> &pdu_block);

  private:
    ola::network::ConnectedDescriptor *m_descriptor;
    PreamblePacker *m_packer;
    bool m_free_packer;

    TCPTransport(const TCPTransport&);
    TCPTransport& operator=(const TCPTransport&);
};


/**
 * Read ACN messages from a stream. Generally you want to use the
 * IncomingTCPTransport directly. This class is used for testing.
 */
class IncommingStreamTransport {
  public:
    /**
     * The E1.33 headers require an IP address and a port
     */
    IncommingStreamTransport(class BaseInflator *inflator,
                             ola::network::ConnectedDescriptor *descriptor,
                             const IPV4Address &ip_address,
                             uint16_t port);
    ~IncommingStreamTransport();

    void Receive();

  private:
    TransportHeader m_transport_header;
    class BaseInflator *m_inflator;
    ola::network::ConnectedDescriptor *m_descriptor;
    // end points to the byte after the data
    uint8_t *m_buffer_start, *m_buffer_end, *m_data_start, *m_data_end;

    void IncreaseBufferSize(unsigned int new_size);
    bool ReadChunk();
    void LookAheadForHeader();
    void RealignBuffer();

    /**
     * Return the available space in the buffer, this will not be contiguous
     * unless m_data_start == m_buffer_start
     */
    inline unsigned int AvailableBufferSpace() const {
      return ContiguousSpace() + (
          m_data_start ? m_data_start - m_buffer_start : 0);
    }

    /**
     * Returns the free contiguous space at the end of the buffer.
     */
    inline unsigned int ContiguousSpace() const {
      return m_data_start ? m_buffer_end - m_data_end : 0;
    }

    /**
     * Return the amount of data in the buffer
     */
    inline unsigned int DataLength() const {
      return m_data_start ? m_data_end - m_data_start : 0;
    }

    /**
     * Return the size of the buffer
     */
    inline unsigned int BufferSize() const {
      return m_buffer_end - m_buffer_start;
    }

    // TODO(simon): tune this once we have an idea of what the sizes will be
    static const unsigned int INITIAL_SIZE = 500;
    static const unsigned int ACN_HEADER_SIZE;
};


/**
 * IncomingTCPTransport is responsible for receiving ACN over TCP.
 */
class IncomingTCPTransport {
  public:
    IncomingTCPTransport(class BaseInflator *inflator,
                         ola::network::TcpSocket *socket);
    ~IncomingTCPTransport() {}

    void Receive() { m_transport->Receive(); }

  private:
    std::auto_ptr<IncommingStreamTransport> m_transport;
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_TCPTRANSPORT_H_
