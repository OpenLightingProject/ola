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
#include "tools/slp/SLPPacketParser.h"
#include "tools/slp/SLPUDPSender.h"
#include "tools/slp/ScopedSLPStore.h"
#include "tools/slp/URLEntry.h"

using ola::io::IOQueue;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::TCPSocket;
using std::auto_ptr;
using std::string;
using std::vector;
using std::set;

namespace ola {

namespace http {
  class OlaHTTPServer;
}

namespace slp {

class SLPServiceImpl;

/**
 * Capture events from stdin and pass them to the SLPServer to act on
 */
class StdinHandler: public ola::io::StdinHandler {
  public:
    StdinHandler(ola::io::SelectServer *ss, class SLPServer *slp_server)
        : ola::io::StdinHandler(ss),
          m_slp_server(slp_server) {
    }

    void HandleCharacter(char c);

  private:
    class SLPServer *m_slp_server;
};


/**
 * An SLP Server.
 * TODO(simon): split this into a daemon and a server component.
 */
class SLPServer {
  public:
    struct SLPServerOptions {
      // IP to multicast on
      IPV4Address ip_address;  // The interface IP to multicast on
      bool enable_da;  // enable the DA mode
      bool enable_http;  // enable the HTTP server
      uint16_t http_port;  // port to run the HTTP server on
      uint16_t rpc_port;  // port to run the RPC server on
      uint32_t config_da_beat;  // seconds between DA beats
      set<string> scopes;  // supported scopes

      SLPServerOptions()
          : enable_da(true),
            enable_http(true),
            http_port(DEFAULT_SLP_HTTP_PORT),
            rpc_port(DEFAULT_SLP_RPC_PORT),
            config_da_beat(3 * 60 * 60) {
      }
    };

    SLPServer(ola::network::UDPSocket *udp_socket,
              ola::network::TCPAcceptingSocket *tcp_socket,
              const SLPServerOptions &options,
              ola::ExportMap *export_map);
    ~SLPServer();

    bool Init();
    void Run();
    void Stop();

    // bulk load a set of URLEntries
    void BulkLoad(const string &scope,
                  const string &service,
                  const URLEntries &entries);

    // handle events from stdin
    void Input(char c);

    static const uint16_t DEFAULT_SLP_PORT;

  private:
    bool m_enable_da;
    uint32_t m_config_da_beat;
    string m_en_lang;

    const IPV4Address m_iface_address;
    ola::Clock m_clock;
    ola::TimeStamp m_boot_time;
    ola::io::SelectServer m_ss;
    StdinHandler m_stdin_handler;

    // RPC members
    const uint16_t m_rpc_port;
    ola::network::TCPSocketFactory m_rpc_socket_factory;
    ola::network::TCPAcceptingSocket m_rpc_accept_socket;
    auto_ptr<ola::network::IPV4SocketAddress> m_multicast_endpoint;
    auto_ptr<SLPServiceImpl> m_service_impl;

    // the UDP and TCP sockets for SLP traffic
    ola::network::UDPSocket *m_udp_socket;
    ola::network::TCPAcceptingSocket *m_slp_accept_socket;

    // SLP memebers
    set<string> m_scope_list;  // the scopes we're using, in canonical form
    SLPPacketParser m_packet_parser;
    ScopedSLPStore m_service_store;
    SLPUDPSender m_udp_sender;

    // DA members

    // non-DA members
    set<IPV4Address> m_da_pr_list;

    // The ExportMap & HTTPServer
    ola::ExportMap *m_export_map;
    auto_ptr<ola::http::OlaHTTPServer> m_http_server;

    // Random methods
    void DumpStore();

    // RPC methods
    void NewTCPConnection(TCPSocket *socket);
    void RPCSocketClosed(TCPSocket *socket);

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

    /*
    bool PerformSanityChecks(const SLPPacket *packet,
                             const IPV4SocketAddress &source);
    */

    void SendErrorIfUnicast(const ServiceRequestPacket *request,
                            const IPV4SocketAddress &source,
                            slp_error_code_t error_code);

    // DA methods
    bool SendDABeat();

    // non-DA methods
    bool SendFindDAService();

    static const char CONFIG_DA_BEAT_VAR[];
    static const char DA_ENABLED_VAR[];
    static const char DA_SERVICE[];
    static const char DEFAULT_SCOPE[];
    static const char SCOPE_LIST_VAR[];
    static const char SLP_PORT_VAR[];
    static const char SERVICE_AGENT_SERVICE[];
    static const char DIRECTORY_AGENT_SERVICE[];
    static const uint16_t DEFAULT_SLP_HTTP_PORT;
    static const uint16_t DEFAULT_SLP_RPC_PORT;
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPSERVER_H_
