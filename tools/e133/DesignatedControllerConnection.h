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
 * DesignatedControllerConnection.h
 * Handles the connection to a designated controller.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef TOOLS_E133_DESIGNATEDCONTROLLERCONNECTION_H_
#define TOOLS_E133_DESIGNATEDCONTROLLERCONNECTION_H_

#include <string>

#include "ola/io/SelectServerInterface.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/Socket.h"
#include "ola/network/TCPSocket.h"
#include "ola/network/TCPSocketFactory.h"
#include "plugins/e131/e131/E133Inflator.h"
#include "plugins/e131/e131/E133StatusInflator.h"
#include "plugins/e131/e131/RootInflator.h"
#include "plugins/e131/e131/TCPTransport.h"

#include "tools/e133/E133HealthCheckedConnection.h"
#include "tools/e133/MessageBuilder.h"
#include "tools/e133/MessageQueue.h"
#include "tools/e133/TCPConnectionStats.h"
#include "tools/e133/TCPMessageSender.h"

using std::string;

class DesignatedControllerConnection {
  public:
    DesignatedControllerConnection(ola::io::SelectServerInterface *ss,
                                   const ola::network::IPV4Address &ip_address,
                                   MessageBuilder *message_builder,
                                   TCPConnectionStats *tcp_stats);
    ~DesignatedControllerConnection();

    bool Init();

    // Call this to send RDMResponses (i.e. Queued Messages) to the designated
    // controller.
    bool SendStatusMessage(const ola::rdm::RDMResponse *response);

    bool CloseTCPConnection();

  private:
    const ola::network::IPV4Address m_ip_address;
    ola::io::SelectServerInterface *m_ss;
    MessageBuilder *m_message_builder;
    TCPConnectionStats *m_tcp_stats;

    // TCP connection classes
    ola::network::TCPSocket *m_tcp_socket;
    E133HealthCheckedConnection *m_health_checked_connection;
    MessageQueue *m_message_queue;
    TCPMessageSender m_tcp_message_sender;
    ola::plugin::e131::IncomingTCPTransport *m_incoming_tcp_transport;

    // Listening Socket
    ola::network::TCPSocketFactory m_tcp_socket_factory;
    ola::network::TCPAcceptingSocket m_listening_tcp_socket;

    // Inflators
    ola::plugin::e131::RootInflator m_root_inflator;
    ola::plugin::e131::E133Inflator m_e133_inflator;
    ola::plugin::e131::E133StatusInflator m_e133_status_inflator;

    void NewTCPConnection(ola::network::TCPSocket *socket);
    void ReceiveTCPData();
    void TCPConnectionUnhealthy();
    void TCPConnectionClosed();
    void RLPDataReceived(const ola::plugin::e131::TransportHeader &header);

    void SendStatusMessage(const ola::network::IPV4SocketAddress target,
                           uint32_t sequence_number,
                           uint16_t endpoint_id,
                           ola::plugin::e131::E133StatusCode status_code,
                           const string &description);

    void HandleStatusMessage(
        const ola::plugin::e131::TransportHeader &transport_header,
        const ola::plugin::e131::E133Header &e133_header,
        uint16_t status_code,
        const string &description);
};
#endif  // TOOLS_E133_DESIGNATEDCONTROLLERCONNECTION_H_
