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
 * DesignatedControllerConnection.h
 * Handles the connection to a designated controller.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef TOOLS_E133_DESIGNATEDCONTROLLERCONNECTION_H_
#define TOOLS_E133_DESIGNATEDCONTROLLERCONNECTION_H_

#include <map>
#include <string>

#include "ola/base/Macro.h"
#include "ola/e133/MessageBuilder.h"
#include "ola/io/NonBlockingSender.h"
#include "ola/io/SelectServerInterface.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/Socket.h"
#include "ola/network/TCPSocket.h"
#include "ola/network/TCPSocketFactory.h"
#include "ola/util/SequenceNumber.h"
#include "libs/acn/E133HealthCheckedConnection.h"
#include "libs/acn/E133Inflator.h"
#include "libs/acn/E133StatusInflator.h"
#include "libs/acn/RootInflator.h"
#include "libs/acn/TCPTransport.h"
#include "tools/e133/TCPConnectionStats.h"

using std::string;

class DesignatedControllerConnection {
 public:
    DesignatedControllerConnection(
        ola::io::SelectServerInterface *ss,
        const ola::network::IPV4Address &ip_address,
        ola::e133::MessageBuilder *message_builder,
        TCPConnectionStats *tcp_stats,
        unsigned int max_queue_size = MAX_QUEUE_SIZE);
    ~DesignatedControllerConnection();

    bool Init();

    // Call this to send RDMResponses (i.e. Queued Messages) to the designated
    // controller.
    bool SendStatusMessage(
      uint16_t endpoint,
      const ola::rdm::RDMResponse *response);

    bool CloseTCPConnection();

 private:
    const ola::network::IPV4Address m_ip_address;
    const unsigned int m_max_queue_size;
    ola::io::SelectServerInterface *m_ss;
    ola::e133::MessageBuilder *m_message_builder;
    TCPConnectionStats *m_tcp_stats;

    // TCP connection classes
    ola::network::TCPSocket *m_tcp_socket;
    E133HealthCheckedConnection *m_health_checked_connection;
    ola::io::NonBlockingSender *m_message_queue;
    ola::acn::IncomingTCPTransport *m_incoming_tcp_transport;

    // Listening Socket
    ola::network::TCPSocketFactory m_tcp_socket_factory;
    ola::network::TCPAcceptingSocket m_listening_tcp_socket;

    // Inflators
    ola::acn::RootInflator m_root_inflator;
    ola::acn::E133Inflator m_e133_inflator;
    ola::acn::E133StatusInflator m_e133_status_inflator;

    // The message state.
    // Indicates if we have messages that haven't been sent on the
    // NonBlockingSender yet.
    typedef std::map<unsigned int, class OutstandingMessage*> PendingMessageMap;
    bool m_unsent_messages;
    PendingMessageMap m_unacked_messages;
    ola::SequenceNumber<unsigned int> m_sequence_number;

    void NewTCPConnection(ola::network::TCPSocket *socket);
    void ReceiveTCPData();
    void TCPConnectionUnhealthy();
    void TCPConnectionClosed();
    void RLPDataReceived(const ola::acn::TransportHeader &header);

    bool SendRDMCommand(unsigned int sequence_number, uint16_t endpoint,
                        const ola::rdm::RDMResponse *rdm_response);

    void HandleStatusMessage(
        const ola::acn::TransportHeader *transport_header,
        const ola::acn::E133Header *e133_header,
        uint16_t status_code,
        const string &description);

    static const unsigned int MAX_QUEUE_SIZE;

    DISALLOW_COPY_AND_ASSIGN(DesignatedControllerConnection);
};
#endif  // TOOLS_E133_DESIGNATEDCONTROLLERCONNECTION_H_
