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
 * SLPDaemon.h
 * Wraps the SLP server and provides an RPC interface. Also runs the embedded
 * webserver.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_SLPDAEMON_H_
#define TOOLS_SLP_SLPDAEMON_H_

#include <ola/Clock.h>
#include <ola/ExportMap.h>
#include <ola/io/SelectServer.h>
#include <ola/io/StdinHandler.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/Socket.h>
#include <ola/network/SocketAddress.h>
#include <ola/network/TCPSocketFactory.h>

#include <iostream>
#include <memory>
#include <string>

#include "tools/slp/SLPServer.h"
#include "tools/slp/SLP.pb.h"

using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::TCPSocket;
using std::auto_ptr;
using std::string;

namespace ola {

namespace http {
  class OlaHTTPServer;
}

namespace slp {

class SLPServiceImpl;

/**
 * Capture events from stdin and pass them to the SLPDaemon to act on
 */
class StdinHandler: public ola::io::StdinHandler {
  public:
    StdinHandler(ola::io::SelectServer *ss, class SLPDaemon *slp_server)
        : ola::io::StdinHandler(ss),
          m_slp_server(slp_server) {
    }

    void HandleCharacter(char c);

  private:
    class SLPDaemon *m_slp_server;
};


/**
 * An SLP Daemon.
 */
class SLPDaemon {
  public:
    struct SLPDaemonOptions: public SLPServer::SLPServerOptions {
      // IP to multicast on
      bool enable_http;  // enable the HTTP server
      uint16_t http_port;  // port to run the HTTP server on
      uint16_t rpc_port;  // port to run the RPC server on

      SLPDaemonOptions()
          : SLPServerOptions(),
            enable_http(true),
            http_port(DEFAULT_SLP_HTTP_PORT),
            rpc_port(DEFAULT_SLP_RPC_PORT) {
      }
    };

    SLPDaemon(ola::network::UDPSocket *udp_socket,
              ola::network::TCPAcceptingSocket *tcp_socket,
              const SLPDaemonOptions &options,
              ola::ExportMap *export_map);
    ~SLPDaemon();

    bool Init();
    void Run();
    void Stop();

    // bulk load a set of URLEntries
    void BulkLoad(const string &scope,
                  const string &service,
                  const URLEntries &entries);

    // handle events from stdin
    void Input(char c);

  private:
    /**
     * The implementation of the SLP Service.
     */
    class SLPServiceImpl : public ola::slp::proto::SLPService {
      public:
        explicit SLPServiceImpl(SLPServer *server)
            : m_slp_server(server) {
        }
        ~SLPServiceImpl() {}

        void FindService(::google::protobuf::RpcController* controller,
                         const ola::slp::proto::ServiceRequest* request,
                         ola::slp::proto::ServiceReply* response,
                         ::google::protobuf::Closure* done);

        void RegisterService(
            ::google::protobuf::RpcController* controller,
            const ola::slp::proto::ServiceRegistration* request,
            ola::slp::proto::ServiceAck* response,
            ::google::protobuf::Closure* done);

        void DeRegisterService(
            ::google::protobuf::RpcController* controller,
            const ola::slp::proto::ServiceDeRegistration* request,
            ola::slp::proto::ServiceAck* response,
            ::google::protobuf::Closure* done);

      private:
        SLPServer *m_slp_server;

        void FindServiceHandler(ola::slp::proto::ServiceReply* response,
                                ::google::protobuf::Closure* done,
                                const URLEntries &urls);

        void RegisterServiceHandler(ola::slp::proto::ServiceAck* response,
                                    ::google::protobuf::Closure* done,
                                    unsigned int error_code);
    };

    ola::Clock m_clock;
    ola::io::SelectServer m_ss;
    SLPServer m_slp_server;
    StdinHandler m_stdin_handler;

    // RPC members
    const uint16_t m_rpc_port;
    ola::network::TCPSocketFactory m_rpc_socket_factory;
    ola::network::TCPAcceptingSocket m_rpc_accept_socket;
    auto_ptr<ola::network::IPV4SocketAddress> m_multicast_endpoint;
    auto_ptr<SLPServiceImpl> m_service_impl;

    // The ExportMap & HTTPServer
    ola::ExportMap *m_export_map;
    auto_ptr<ola::http::OlaHTTPServer> m_http_server;

    // Random methods
    void DumpStore();

    // RPC methods
    void NewTCPConnection(TCPSocket *socket);
    void RPCSocketClosed(TCPSocket *socket);

    static const uint16_t DEFAULT_SLP_HTTP_PORT;
    static const uint16_t DEFAULT_SLP_RPC_PORT;
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPDAEMON_H_
