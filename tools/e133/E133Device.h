/*
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
 * E133Device.h
 * The main E1.33 class.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef TOOLS_E133_E133DEVICE_H_
#define TOOLS_E133_E133DEVICE_H_


#include <string>
#include <vector>

#include "ola/Clock.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/SelectServerInterface.h"
#include "ola/network/Socket.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "plugins/e131/e131/CID.h"
#include "plugins/e131/e131/DMPE133Inflator.h"
#include "plugins/e131/e131/E133Inflator.h"
#include "plugins/e131/e131/E133Sender.h"
#include "plugins/e131/e131/RootInflator.h"
#include "plugins/e131/e131/RootSender.h"
#include "plugins/e131/e131/TCPTransport.h"
#include "plugins/e131/e131/UDPTransport.h"

#include "tools/e133/E133Endpoint.h"
#include "tools/e133/E133HealthCheckedConnection.h"

using std::string;
using std::auto_ptr;

/**
 * A E1.33 Device.
 * E1.33 Devices can either be native, or gateways to E1.20 devices.
 */
class E133Device {
  public:
    E133Device(ola::network::SelectServerInterface *ss,
               const ola::network::IPV4Address &ip_address,
               class EndpointManager *endpoint_manager,
               class TCPConnectionStats *tcp_stats);
    ~E133Device();

    void SetRootEndpoint(E133EndpointInterface *endpoint);

    bool Init();

  private:
    class EndpointManager *m_endpoint_manager;
    auto_ptr<ola::Callback1<void, uint16_t> > m_register_endpoint_callback;
    auto_ptr<ola::Callback1<void, uint16_t> > m_unregister_endpoint_callback;
    E133EndpointInterface *m_root_endpoint;

    class TCPConnectionStats *m_tcp_stats;

    // The Node's CID
    ola::plugin::e131::CID m_cid;

    // Frequency of TCP health checking
    ola::TimeInterval m_health_check_interval;
    ola::network::ConnectedDescriptor *m_tcp_descriptor;
    E133HealthCheckedConnection *m_health_checked_connection;

    // the RDM device to handle requests to the Root Endpoint
    ola::rdm::RDMControllerInterface *m_root_rdm_device;

    // network members
    const string m_preferred_ip;
    ola::network::SelectServerInterface *m_ss;
    ola::network::IPV4Address m_ip_address;
    ola::network::UdpSocket m_udp_socket;
    ola::network::TcpAcceptingSocket m_tcp_socket;

    // inflators
    ola::plugin::e131::RootInflator m_root_inflator;
    ola::plugin::e131::E133Inflator m_e133_inflator;
    ola::plugin::e131::DMPE133Inflator m_dmp_inflator;

    // transports
    ola::plugin::e131::IncomingUDPTransport m_incoming_udp_transport;
    ola::plugin::e131::OutgoingUDPTransportImpl m_outgoing_udp_transport;
    ola::plugin::e131::IncomingTCPTransport *m_incoming_tcp_transport;

    // senders
    ola::plugin::e131::RootSender m_root_sender;
    ola::plugin::e131::E133Sender m_e133_sender;

    void NewTCPConnection(ola::network::TcpSocket *descriptor);
    void TCPConnectionUnhealthy();
    void TCPConnectionClosed();
    void E133DataReceived(const ola::plugin::e131::TransportHeader &header);

    void RegisterEndpoint(uint16_t endpoint_id);
    void UnRegisterEndpoint(uint16_t endpoint_id);

    void EndpointRequest(
        uint16_t endpoint_id,
        const ola::plugin::e131::TransportHeader &transport_header,
        const ola::plugin::e131::E133Header &e133_header,
        const string &raw_request);

    void EndpointRequestComplete(ola::network::IPV4Address src_ip,
                                 uint16_t src_port,
                                 uint32_t sequence_number,
                                 uint16_t endpoint_id,
                                 ola::rdm::rdm_response_code response_code,
                                 const ola::rdm::RDMResponse *response,
                                 const std::vector<string> &packets);
};
#endif  // TOOLS_E133_E133DEVICE_H_
