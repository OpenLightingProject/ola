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
 * TCPTransport.h
 * Copyright (C) 2012 Simon Newton
 *
 * This defines the OutgoingStreamTransport and IncommingStreamTransport for
 * sending and receiving PDUs over stream connections.
 *
 * When receiving, the BaseInflator is passed a header containing the source IP
 * & port (since many higher layer protocols require this). When using the
 * IncommingStreamTransport you need to provide a fake ip:port pair.
 *
 * It's unlikely you want to use IncomingTCPTransport directly, since all
 * real world connections are TCP (rather than pipes etc.). The
 * IncommingStreamTransport is separate because it assists in testing.
 */

#ifndef PLUGINS_E131_E131_TCPTRANSPORT_H_
#define PLUGINS_E131_E131_TCPTRANSPORT_H_

#include <memory>
#include "ola/io/OutputBuffer.h"
#include "ola/io/OutputStream.h"
#include "ola/io/Descriptor.h"
#include "ola/network/TCPSocket.h"
#include "plugins/e131/e131/PDU.h"
#include "plugins/e131/e131/Transport.h"

namespace ola {
namespace plugin {
namespace e131 {


/**
 * Read ACN messages from a stream. Generally you want to use the
 * IncomingTCPTransport directly. This class is used for testing.
 */
class IncommingStreamTransport {
 public:
    IncommingStreamTransport(class BaseInflator *inflator,
                             ola::io::ConnectedDescriptor *descriptor,
                             const ola::network::IPV4SocketAddress &source);
    ~IncommingStreamTransport();

    bool Receive();

 private:
    // The receiver is a state machine.
    typedef enum {
      WAITING_FOR_PREAMBLE,
      WAITING_FOR_PDU_FLAGS,
      WAITING_FOR_PDU_LENGTH,
      WAITING_FOR_PDU,
    } RXState;

    typedef enum {
      TWO_BYTES = 2,
      THREE_BYTES = 3,
    } PDULengthSize;

    TransportHeader m_transport_header;
    class BaseInflator *m_inflator;
    ola::io::ConnectedDescriptor *m_descriptor;

    // end points to the byte after the data
    uint8_t *m_buffer_start, *m_buffer_end, *m_data_end;
    // the amount of data we need before we can move to the next stage
    unsigned int m_outstanding_data;
    // the state we're currently in
    RXState m_state;
    unsigned int m_block_size;
    unsigned int m_consumed_block_size;
    bool m_stream_valid;
    PDULengthSize m_pdu_length_size;
    unsigned int m_pdu_size;

    void HandlePreamble();
    void HandlePDUFlags();
    void HandlePDULength();
    void HandlePDU();

    void IncreaseBufferSize(unsigned int new_size);
    void ReadRequiredData();
    void EnterWaitingForPreamble();
    void EnterWaitingForPDU();

    /**
     * Returns the free space at the end of the buffer.
     */
    inline unsigned int FreeSpace() const {
      return m_buffer_start ?
        static_cast<unsigned int>(m_buffer_end - m_data_end) : 0u;
    }

    /**
     * Return the amount of data in the buffer
     */
    inline unsigned int DataLength() const {
      return m_buffer_start ?
        static_cast<unsigned int>(m_data_end - m_buffer_start) : 0u;
    }

    /**
     * Return the size of the buffer
     */
    inline unsigned int BufferSize() const {
      return static_cast<unsigned int>(m_buffer_end - m_buffer_start);
    }

    static const unsigned int INITIAL_SIZE;
    static const unsigned int PDU_BLOCK_SIZE = 4;
};


/**
 * IncomingTCPTransport is responsible for receiving ACN over TCP.
 */
class IncomingTCPTransport {
 public:
    IncomingTCPTransport(class BaseInflator *inflator,
                         ola::network::TCPSocket *socket);
    ~IncomingTCPTransport() {}

    bool Receive() { return m_transport->Receive(); }

 private:
    std::auto_ptr<IncommingStreamTransport> m_transport;
};
}  // namespace e131
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_E131_E131_TCPTRANSPORT_H_
