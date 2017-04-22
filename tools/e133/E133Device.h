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
 * E133Device.h
 * Encapsulates the functionality of an E1.33 device.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef TOOLS_E133_E133DEVICE_H_
#define TOOLS_E133_E133DEVICE_H_

#include <string>
#include <vector>

#include "ola/Clock.h"
#include "ola/acn/CID.h"
#include "ola/e133/MessageBuilder.h"
#include "ola/io/SelectServerInterface.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/Socket.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "libs/acn/E133Inflator.h"
#include "libs/acn/RDMInflator.h"
#include "libs/acn/RootInflator.h"
#include "libs/acn/UDPTransport.h"

#include "tools/e133/DesignatedControllerConnection.h"
#include "tools/e133/E133Endpoint.h"
#include "tools/e133/TCPConnectionStats.h"

using std::string;
using std::auto_ptr;

/**
 * This encapulates the functionality of an E1.33 Device.
 * E1.33 Devices can either be native, or gateways to E1.20 devices.
 */
class E133Device {
 public:
    E133Device(ola::io::SelectServerInterface *ss,
               const ola::acn::CID &cid,
               const ola::network::IPV4Address &ip_address,
               class EndpointManager *endpoint_manager);
    ~E133Device();

    void SetRootEndpoint(E133EndpointInterface *endpoint);

    bool Init();

    TCPConnectionStats* GetTCPStats();
    void SendStatusMessage(const ola::rdm::RDMResponse *response);
    bool CloseTCPConnection();

 private:
    ola::io::SelectServerInterface *m_ss;
    const ola::network::IPV4Address m_ip_address;
    ola::e133::MessageBuilder m_message_builder;
    TCPConnectionStats m_tcp_stats;
    auto_ptr<DesignatedControllerConnection> m_controller_connection;

    class EndpointManager *m_endpoint_manager;
    E133EndpointInterface *m_root_endpoint;

    // The RDM device to handle requests to the Root Endpoint
    ola::rdm::RDMControllerInterface *m_root_rdm_device;

    // Network members
    ola::network::UDPSocket m_udp_socket;

    // inflators
    ola::acn::RootInflator m_root_inflator;
    ola::acn::E133Inflator m_e133_inflator;
    ola::acn::RDMInflator m_rdm_inflator;

    // transports
    ola::acn::IncomingUDPTransport m_incoming_udp_transport;

    void EndpointRequest(
        const ola::acn::TransportHeader *transport_header,
        const ola::acn::E133Header *e133_header,
        const string &raw_request);

    void EndpointRequestComplete(ola::network::IPV4SocketAddress target,
                                 uint32_t sequence_number,
                                 uint16_t endpoint_id,
                                 ola::rdm::RDMReply *reply);

    void SendStatusMessage(const ola::network::IPV4SocketAddress target,
                           uint32_t sequence_number,
                           uint16_t endpoint_id,
                           ola::e133::E133StatusCode status_code,
                           const string &description);
};
#endif  // TOOLS_E133_E133DEVICE_H_
