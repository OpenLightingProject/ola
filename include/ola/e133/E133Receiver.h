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
 * E133Receiver.h
 * Handles E1.33 UDP packets and executes RDM and Status message callbacks.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_E133_E133RECEIVER_H_
#define INCLUDE_OLA_E133_E133RECEIVER_H_

#include <ola/Callback.h>
#include <ola/base/Macro.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/Socket.h>
#include <ola/rdm/RDMCommand.h>

#include <memory>
#include <string>

using std::string;
using std::auto_ptr;
using ola::network::IPV4Address;

namespace ola {
// We don't want to suck in all the ACN headers, so we forward declare these
// and allocate the members on the heap.
namespace acn {
class E133Header;
class E133Inflator;
class E133StatusInflator;
class IncomingUDPTransport;
class RDMInflator;
class RootInflator;
class TransportHeader;
}  // namespace acn

namespace e133 {

class E133Message {
 public:
    E133Message(const IPV4Address &ip,
                uint16_t endpoint,
                uint32_t sequence_number)
      : ip(ip),
        endpoint(endpoint),
        sequence_number(sequence_number) {
    }
    virtual ~E133Message() {}

    IPV4Address ip;
    uint16_t endpoint;
    uint32_t sequence_number;
};


/**
 * Wraps a E1.33 Status message.
 */
class E133StatusMessage : public E133Message {
 public:
    E133StatusMessage(const IPV4Address &ip,
                      uint16_t endpoint,
                      uint32_t sequence_number,
                      uint16_t status_code,
                      string status_message)
      : E133Message(ip, endpoint, sequence_number),
        status_code(status_code),
        status_message(status_message) {
    }

    uint16_t status_code;
    string status_message;
};


/**
 * Wraps a RDM message
 * TODO(simon): sort out ownership here
 */
class E133RDMMessage : public E133Message {
 public:
    E133RDMMessage(const IPV4Address &ip,
                   uint16_t endpoint,
                   uint32_t sequence_number,
                   ola::rdm::RDMStatusCode status_code,
                   const ola::rdm::RDMResponse *response)
      : E133Message(ip, endpoint, sequence_number),
        status_code(status_code),
        response(response) {
    }

    ola::rdm::RDMStatusCode status_code;
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
    ~E133Receiver();

 private:
    ola::network::UDPSocket *m_udp_socket;
    StatusCallback *m_status_callback;
    RDMCallback *m_rdm_callback;

    auto_ptr<ola::acn::RootInflator> m_root_inflator;
    auto_ptr<ola::acn::E133Inflator> m_e133_inflator;
    auto_ptr<ola::acn::RDMInflator> m_rdm_inflator;
    auto_ptr<ola::acn::E133StatusInflator> m_e133_status_inflator;
    auto_ptr<ola::acn::IncomingUDPTransport> m_incoming_udp_transport;

    void HandleStatusMessage(
        const ola::acn::TransportHeader *transport_header,
        const ola::acn::E133Header *e133_header,
        uint16_t status_code,
        const string &description);

    void HandlePacket(
        const ola::acn::TransportHeader *transport_header,
        const ola::acn::E133Header *e133_header,
        const std::string &raw_response);

    DISALLOW_COPY_AND_ASSIGN(E133Receiver);
};
}  // namespace e133
}  // namespace ola
#endif  // INCLUDE_OLA_E133_E133RECEIVER_H_
