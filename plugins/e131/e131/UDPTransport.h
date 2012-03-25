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
 * UDPTransport.h
 * Interface for the UDPTransport class
 * Copyright (C) 2007-2009 Simon Newton
 */

#ifndef PLUGINS_E131_E131_UDPTRANSPORT_H_
#define PLUGINS_E131_E131_UDPTRANSPORT_H_

#include "ola/network/IPV4Address.h"
#include "ola/network/Socket.h"
#include "plugins/e131/e131/ACNPort.h"
#include "plugins/e131/e131/PDU.h"
#include "plugins/e131/e131/PreamblePacker.h"
#include "plugins/e131/e131/Transport.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::IPV4Address;

/*
 * The OutgoingUDPTransport is a small shim that provides the options to
 * UDPTransportImpl.
 */
class OutgoingUDPTransport: public OutgoingTransport {
  public:
    OutgoingUDPTransport(class OutgoingUDPTransportImpl *impl,
                         const IPV4Address &destination,
                         uint16_t port = ACN_PORT)
        : m_impl(impl),
          m_destination(destination),
          m_port(port) {
    }
    ~OutgoingUDPTransport() {}

    bool Send(const PDUBlock<PDU> &pdu_block);

  private:
    class OutgoingUDPTransportImpl *m_impl;
    IPV4Address m_destination;
    uint16_t m_port;

    OutgoingUDPTransport(const OutgoingUDPTransport&);
    OutgoingUDPTransport& operator=(const OutgoingUDPTransport&);
};


/**
 * OutgoingUDPTransportImpl is the class that actually does the sending.
 */
class OutgoingUDPTransportImpl {
  public:
    OutgoingUDPTransportImpl(ola::network::UdpSocket *socket,
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
              const IPV4Address &destination,
              uint16_t port);

  private:
    ola::network::UdpSocket *m_socket;
    PreamblePacker *m_packer;
    bool m_free_packer;
};


/**
 * IncomingUDPTransport is responsible for receiving over UDP
 */
class IncomingUDPTransport {
  public:
    IncomingUDPTransport(ola::network::UdpSocket *socket,
                         class BaseInflator *inflator);
    ~IncomingUDPTransport() {
      if (m_recv_buffer)
        delete[] m_recv_buffer;
    }

    void Receive();

  private:
    ola::network::UdpSocket *m_socket;
    class BaseInflator *m_inflator;
    uint8_t *m_recv_buffer;
    uint8_t m_acn_header[PreamblePacker::DATA_OFFSET];
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_UDPTRANSPORT_H_
