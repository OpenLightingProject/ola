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
 * E133Receiver.h
 * Handles E1.33 UDP packets and executes RDM and Status message callbacks.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_E133_E133RECEIVER_H_
#define INCLUDE_OLA_E133_E133RECEIVER_H_

#include <memory>
#include <string>

#include <ola/Callback.h>
#include <ola/network/Socket.h>
#include <ola/network/IPV4Address.h>
#include <ola/rdm/RDMCommand.h>

using std::string;
using std::auto_ptr;
using ola::network::IPV4Address;

namespace ola {
// We don't want to suck in all the ACN headers, so we forward declare these
// and allocate the members on the heap.
namespace plugin {
namespace e131 {
  class E133Header;
  class E133Inflator;
  class E133StatusInflator;
  class IncomingUDPTransport;
  class RDMInflator;
  class RootInflator;
  class TransportHeader;
}  // e131
}  // plugin

namespace e133 {

/**
 * Wraps a E1.33 Status message.
 */
class E133StatusMessage {
  public:
    E133StatusMessage(const IPV4Address &device,
                      uint16_t endpoint,
                      uint32_t sequence_number,
                      uint16_t status_code,
                      string status_message)
      : device(device),
        endpoint(endpoint),
        sequence_number(sequence_number),
        status_code(status_code),
        status_message(status_message) {
    }

    IPV4Address device;
    uint16_t endpoint;
    uint32_t sequence_number;
    uint16_t status_code;
    string status_message;
};


/**
 * Wraps a RDM message
 */
class E133RDMMessage {
  public:
    E133RDMMessage(const IPV4Address &device,
                   uint16_t endpoint,
                   uint32_t sequence_number,
                   const ola::rdm::RDMResponse *response)
      : device(device),
        endpoint(endpoint),
        sequence_number(sequence_number),
        response(response) {
    }

    IPV4Address device;
    uint16_t endpoint;
    uint32_t sequence_number;
    const ola::rdm::RDMResponse *response;
};


/**
 * Given a UDP socket, handle all the message extraction.
 */
class E133Receiver {
  public:
    typedef ola::Callback1<void, const E133StatusMessage&> StatusCallback;
    typedef ola::Callback1<void, const E133RDMMessage&> RDMCallback;

    explicit E133Receiver(ola::network::UDPSocket *socket,
                          StatusCallback *status_callback,
                          RDMCallback *rdm_callback);
    ~E133Receiver() {}

  private:
    ola::network::UDPSocket *m_udp_socket;
    StatusCallback *m_status_callback;
    RDMCallback *m_rdm_callback;

    auto_ptr<ola::plugin::e131::RootInflator> m_root_inflator;
    auto_ptr<ola::plugin::e131::E133Inflator> m_e133_inflator;
    auto_ptr<ola::plugin::e131::RDMInflator> m_rdm_inflator;
    auto_ptr<ola::plugin::e131::E133StatusInflator> m_e133_status_inflator;
    auto_ptr<ola::plugin::e131::IncomingUDPTransport> m_incoming_udp_transport;

    void HandleStatusMessage(
        const ola::plugin::e131::TransportHeader *transport_header,
        const ola::plugin::e131::E133Header *e133_header,
        uint16_t status_code,
        const string &description);

    void HandlePacket(
        const ola::plugin::e131::TransportHeader *transport_header,
        const ola::plugin::e131::E133Header *e133_header,
        const std::string &raw_response);
};
}  // e133
}  // ola
#endif  // INCLUDE_OLA_E133_E133RECEIVER_H_
