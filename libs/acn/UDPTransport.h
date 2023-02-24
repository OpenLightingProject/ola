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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * UDPTransport.h
 * Interface for the UDPTransport class
 * Copyright (C) 2007 Simon Newton
 */

#ifndef LIBS_ACN_UDPTRANSPORT_H_
#define LIBS_ACN_UDPTRANSPORT_H_

#include "ola/acn/ACNPort.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/Socket.h"
#include "libs/acn/PDU.h"
#include "libs/acn/PreamblePacker.h"
#include "libs/acn/Transport.h"

namespace ola {
namespace acn {

/*
 * The OutgoingUDPTransport is a small shim that provides the options to
 * UDPTransportImpl.
 */
class OutgoingUDPTransport: public OutgoingTransport {
 public:
    OutgoingUDPTransport(class OutgoingUDPTransportImpl *impl,
                         const ola::network::IPV4Address &destination,
                         uint16_t port = ola::acn::ACN_PORT)
        : m_impl(impl),
          m_destination(destination, port) {
    }
    ~OutgoingUDPTransport() {}

    bool Send(const PDUBlock<PDU> &pdu_block);

 private:
    class OutgoingUDPTransportImpl *m_impl;
    ola::network::IPV4SocketAddress m_destination;

    OutgoingUDPTransport(const OutgoingUDPTransport&);
    OutgoingUDPTransport& operator=(const OutgoingUDPTransport&);
};


/**
 * OutgoingUDPTransportImpl is the class that actually does the sending.
 */
class OutgoingUDPTransportImpl {
 public:
    OutgoingUDPTransportImpl(ola::network::UDPSocket *socket,
                             PreamblePacker *packer = NULL)
        : m_socket(socket),
          m_packer(packer),
          m_free_packer(false) {
      if (!m_packer) {
        m_packer = new PreamblePacker();
        m_free_packer = true;
      }
    }
    ~OutgoingUDPTransportImpl() {
      if (m_free_packer)
        delete m_packer;
    }

    bool Send(const PDUBlock<PDU> &pdu_block,
              const ola::network::IPV4SocketAddress &destination);

 private:
    ola::network::UDPSocket *m_socket;
    PreamblePacker *m_packer;
    bool m_free_packer;
};


/**
 * IncomingUDPTransport is responsible for receiving over UDP
 * TODO(simon): pass the socket as an argument to receive so we can reuse the
 * transport for multiple sockets.
 */
class IncomingUDPTransport {
 public:
    IncomingUDPTransport(ola::network::UDPSocket *socket,
                         class BaseInflator *inflator);
    ~IncomingUDPTransport() {
      if (m_recv_buffer)
        delete[] m_recv_buffer;
    }

    void Receive();

 private:
    ola::network::UDPSocket *m_socket;
    class BaseInflator *m_inflator;
    uint8_t *m_recv_buffer;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_UDPTRANSPORT_H_
