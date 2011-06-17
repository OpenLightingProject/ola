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

#include <string>
#include "ola/network/IPV4Address.h"
#include "ola/network/Interface.h"
#include "ola/network/Socket.h"
#include "plugins/e131/e131/PDU.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::IPV4Address;

/*
 * Used to send and recv PDUs over UDP
 */
class UDPTransport {
  public:
    static const uint16_t ACN_PORT = 5568;

    explicit UDPTransport(uint16_t port = ACN_PORT):
      m_inflator(NULL),
      m_port(port),
      m_send_buffer(NULL),
      m_recv_buffer(NULL) {
    }

    UDPTransport(class BaseInflator *inflator,
                 uint16_t port = ACN_PORT):
      m_inflator(inflator),
      m_port(port),
      m_send_buffer(NULL),
      m_recv_buffer(NULL) {
    }
    ~UDPTransport();

    bool Init(const ola::network::Interface &interface);
    bool Send(const PDUBlock<PDU> &pdu_block,
              const IPV4Address &destination,
              uint16_t port = ACN_PORT);
    ola::network::UdpSocket *GetSocket() { return &m_socket; }
    void SetInflator(class BaseInflator *inflator) { m_inflator = inflator; }
    void Receive();

    bool JoinMulticast(const IPV4Address &group);
    bool LeaveMulticast(const IPV4Address &group);

  private:
    ola::network::UdpSocket m_socket;
    ola::network::Interface m_interface;
    class BaseInflator *m_inflator;
    uint16_t m_port;
    uint8_t *m_send_buffer;
    uint8_t *m_recv_buffer;

    static const char ACN_PACKET_ID[];  // ASC-E1.17\0\0\0
    // TODO(simon): add MTU discovery?
    static const unsigned int MAX_DATAGRAM_SIZE = 1472;
    static const uint16_t PREAMBLE_SIZE = 0x10;
    static const uint16_t POSTABLE_SIZE = 0;
    static const unsigned int PREAMBLE_OFFSET = 4;
    static const unsigned int DATA_OFFSET = PREAMBLE_OFFSET + 12;
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_UDPTRANSPORT_H_
