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
 * SLPServer.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_SLPSERVER_H_
#define TOOLS_SLP_SLPSERVER_H_

#include <ola/Clock.h>
#include <ola/ExportMap.h>
#include <ola/io/BigEndianStream.h>
#include <ola/io/IOQueue.h>
#include <ola/io/SelectServer.h>
#include <ola/io/StdinHandler.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/Socket.h>
#include <ola/network/SocketAddress.h>
#include <ola/network/TCPSocketFactory.h>

#include <iostream>
#include <memory>
#include <set>
#include <map>
#include <string>
#include <vector>

#include "tools/slp/Base.h"
#include "tools/slp/DATracker.h"
#include "tools/slp/SLPPacketParser.h"
#include "tools/slp/SLPStore.h"
#include "tools/slp/SLPUDPSender.h"
#include "tools/slp/ServiceEntry.h"

using ola::io::IOQueue;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::TCPSocket;
using std::auto_ptr;
using std::string;
using std::set;

namespace ola {
namespace slp {


/**
 * An SLP Server. This handles the SLP messages an provides an API to
 * register, deregister and locate services.
 */
class SLPServer {
  public:
    struct SLPServerOptions {
      IPV4Address ip_address;  // The interface IP to multicast on
      bool enable_da;  // enable the DA mode
      uint32_t config_da_beat;  // seconds between DA beats
      set<string> scopes;  // supported scopes

      SLPServerOptions()
          : enable_da(true),
            config_da_beat(3 * 60 * 60) {
      }
    };

    SLPServer(ola::io::SelectServerInterface *ss,
              ola::network::UDPSocket *udp_socket,
              ola::network::TCPAcceptingSocket *tcp_socket,
              ola::ExportMap *export_map,
              const SLPServerOptions &options);
    ~SLPServer();

    bool Init();

    // bulk load a list of Services
    bool BulkLoad(const ServiceEntries &services);
    void DumpStore();
    void GetDirectoryAgents(vector<DirectoryAgent> *output);

    // SLP API
    void FindService(const set<string> &scopes,
                     const string &service,
                     SingleUseCallback1<void, const URLEntries&> *cb);
    void RegisterService(const set<string> &scopes,
                         const string &url,
                         uint16_t lifetime,
                         SingleUseCallback1<void, unsigned int> *cb);
    void DeRegisterService(const set<string> &scopes,
                           const string &url,
                           SingleUseCallback1<void, unsigned int> *cb);

  private:
    bool m_enable_da;
    uint32_t m_config_da_beat;
    string m_en_lang;

    const IPV4Address m_iface_address;
    ola::Clock m_clock;
    ola::TimeStamp m_boot_time;
    ola::io::SelectServerInterface *m_ss;

    // various timers
    ola::thread::timeout_id m_da_beat_timer;
    ola::thread::timeout_id m_store_cleaner_timer;

    // RPC members
    auto_ptr<ola::network::IPV4SocketAddress> m_multicast_endpoint;

    // the UDP and TCP sockets for SLP traffic
    ola::network::UDPSocket *m_udp_socket;
    ola::network::TCPAcceptingSocket *m_slp_accept_socket;

    // SLP memebers
    set<string> m_scope_list;  // the scopes we're using, in canonical form
    SLPPacketParser m_packet_parser;
    SLPStore m_service_store;
    SLPUDPSender m_udp_sender;
    DATracker m_da_tracker;

    // DA members

    // non-DA members
    set<IPV4Address> m_da_pr_list;

    // The ExportMap
    ola::ExportMap *m_export_map;

    // SLP Network methods
    void UDPData();
    void HandleServiceRequest(BigEndianInputStream *stream,
                              const IPV4SocketAddress &source);
    void HandleServiceReply(BigEndianInputStream *stream,
                            const IPV4SocketAddress &source);
    void HandleServiceRegistration(BigEndianInputStream *stream,
                                   const IPV4SocketAddress &source);
    void HandleServiceAck(BigEndianInputStream *stream,
                          const IPV4SocketAddress &source);
    void HandleDAAdvert(BigEndianInputStream *stream,
                        const IPV4SocketAddress &source);

    void SendErrorIfUnicast(const ServiceRequestPacket *request,
                            const IPV4SocketAddress &source,
                            slp_error_code_t error_code);

    void MaybeSendSAAdvert(const ServiceRequestPacket *request,
                           const IPV4SocketAddress &source);

    void MaybeSendDAAdvert(const ServiceRequestPacket *request,
                           const IPV4SocketAddress &source);

    // DA methods
    void SendDAAdvert(const IPV4SocketAddress &dest);
    bool SendDABeat();
    void LocalLocateServices(const set<string> &scopes,
                             const string &service,
                             URLEntries *services);
    uint16_t LocalRegisterService(const ServiceEntry &service);
    uint16_t LocalDeRegisterService(const ServiceEntry &service);

    // non-DA methods
    bool SendFindDAService();
    void NewDACallback(const DirectoryAgent &agent);

    // housekeeping
    bool CleanSLPStore();

    static const char CONFIG_DA_BEAT_VAR[];
    static const char DAADVERT[];
    static const char DA_ENABLED_VAR[];
    static const char DEREGSRVS_ERROR_COUNT_VAR[];
    static const char FINDSRVS_EMPTY_COUNT_VAR[];
    static const char METHOD_CALLS_VAR[];
    static const char METHOD_DEREG_SERVICE[];
    static const char METHOD_FIND_SERVICE[];
    static const char METHOD_REG_SERVICE[];
    static const char REGSRVS_ERROR_COUNT_VAR[];
    static const char SCOPE_LIST_VAR[];
    static const char SLP_PORT_VAR[];
    static const char SRVACK[];
    static const char SRVREG[];
    static const char SRVRPLY[];
    static const char SRVRQST[];
    static const char UDP_RX_PACKET_BY_TYPE_VAR[];
    static const char UDP_RX_TOTAL_VAR[];
    static const char UDP_TX_PACKET_BY_TYPE_VAR[];
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPSERVER_H_
