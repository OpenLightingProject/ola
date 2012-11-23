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
 * SLPDaemon.cpp
 * Copyright (C) 2012 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ola/BaseTypes.h>
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/io/SelectServer.h>
#include <ola/io/BigEndianStream.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/Socket.h>
#include <ola/network/SocketAddress.h>
#include <ola/network/TCPSocketFactory.h>

#ifdef HAVE_LIBMICROHTTPD
#include <ola/http/HTTPServer.h>
#include <ola/http/OlaHTTPServer.h>
#endif

#include <string>
#include <set>
#include <vector>

#include "common/rpc/StreamRpcChannel.h"
#include "tools/slp/SLPDaemon.h"

namespace ola {
namespace slp {

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::TCPAcceptingSocket;
using ola::network::TCPSocket;
using ola::network::UDPSocket;
using ola::rpc::StreamRpcChannel;
using std::auto_ptr;
using std::string;


const uint16_t SLPDaemon::DEFAULT_SLP_HTTP_PORT = 9012;
const uint16_t SLPDaemon::DEFAULT_SLP_RPC_PORT = 9011;

void StdinHandler::HandleCharacter(char c) {
  m_slp_server->Input(c);
}


/**
 * Setup a new SLP server.
 * @param socket the UDP Socket to use for SLP messages.
 * @param options the SLP Server options.
 */
SLPDaemon::SLPDaemon(ola::network::UDPSocket *udp_socket,
                     ola::network::TCPAcceptingSocket *tcp_socket,
                     const SLPDaemonOptions &options,
                     ola::ExportMap *export_map)
    : m_ss(export_map, &m_clock),
      m_slp_server(&m_ss, udp_socket, tcp_socket, export_map, options),
      m_stdin_handler(&m_ss, this),

      m_rpc_port(options.rpc_port),
      m_rpc_socket_factory(NewCallback(this, &SLPDaemon::NewTCPConnection)),
      m_rpc_accept_socket(&m_rpc_socket_factory),
      m_service_impl(new SLPServiceImpl(&m_slp_server)),
      m_export_map(export_map) {
#ifdef HAVE_LIBMICROHTTPD
  if (options.enable_http) {
    ola::http::HTTPServer::HTTPServerOptions http_options;
    http_options.port = options.http_port;

    m_http_server.reset(
        new ola::http::OlaHTTPServer(http_options, m_export_map));
  }
#endif
}


SLPDaemon::~SLPDaemon() {
  m_rpc_accept_socket.Close();
}


/**
 * Init the server
 */
bool SLPDaemon::Init() {
  if (!m_slp_server.Init())
    return false;

  // setup the accepting TCP socket
  if (!m_rpc_accept_socket.Listen(
        IPV4SocketAddress(IPV4Address::Loopback(), m_rpc_port))) {
    return false;
  }

  m_ss.AddReadDescriptor(&m_rpc_accept_socket);

#ifdef HAVE_LIBMICROHTTPD
  if (m_http_server.get())
    m_http_server->Init();
#endif
  return true;
}


void SLPDaemon::Run() {
#ifdef HAVE_LIBMICROHTTPD
  if (m_http_server.get())
    m_http_server->Start();
#endif
  m_ss.Run();
}


void SLPDaemon::Stop() {
#ifdef HAVE_LIBMICROHTTPD
  if (m_http_server.get())
    m_http_server->Stop();
#endif
  m_ss.Terminate();
}


/**
 * Bulk load a set of URL Entries
 */
bool SLPDaemon::BulkLoad(const ServiceEntries &entries) {
  return m_slp_server.BulkLoad(entries);
}


/*
 * Called when there is data on stdin.
 */
void SLPDaemon::Input(char c) {
  switch (c) {
    case 'd':
      GetDirectoryAgents();
      break;
    case 'p':
      DumpStore();
      break;
    case 'q':
      m_ss.Terminate();
      break;
    default:
      break;
  }
}


/**
 * Dump out the contents of the SLP store.
 */
void SLPDaemon::DumpStore() {
  m_slp_server.DumpStore();
}


/**
 * Print a list of DAs
 */
void SLPDaemon::GetDirectoryAgents() {
  vector<DirectoryAgent> agents;
  m_slp_server.GetDirectoryAgents(&agents);
  for (vector<DirectoryAgent>::const_iterator iter = agents.begin();
       iter != agents.end(); ++iter)
    OLA_INFO << *iter;
}


/**
 * Called when RPC client connects.
 */
void SLPDaemon::NewTCPConnection(TCPSocket *socket) {
  IPV4Address peer_address;
  uint16_t port;
  socket->GetPeer(&peer_address, &port);
  OLA_INFO << "New connection from " << peer_address << ":" << port;

  StreamRpcChannel *channel = new StreamRpcChannel(m_service_impl.get(), socket,
                                                   m_export_map);
  socket->SetOnClose(
      NewSingleCallback(this, &SLPDaemon::RPCSocketClosed, socket));

  (void) channel;

  /*
  pair<int, OlaClientService*> pair(socket->ReadDescriptor(), service);
  m_sd_to_service.insert(pair);
  */

  // This hands off ownership to the select server
  m_ss.AddReadDescriptor(socket, true);
  // (*m_export_map->GetIntegerVar(K_CLIENT_VAR))++;
}


/**
 * Called when RPC socket is closed.
 */
void SLPDaemon::RPCSocketClosed(TCPSocket *socket) {
  OLA_INFO << "RPC Socket closed";
  (void) socket;
}

//------------------------------------------------------------------------------
// SLPServiceImpl methods

/**
 * Find Service request.
 */
void SLPDaemon::SLPServiceImpl::FindService(
    ::google::protobuf::RpcController*,
    const ola::slp::proto::ServiceRequest* request,
    ola::slp::proto::ServiceReply* response,
    ::google::protobuf::Closure* done) {
  OLA_INFO << "Recv FindService request";

  set<string> scopes;
  for (int i = 0; i < request->scope_size(); ++i)
    scopes.insert(request->scope(i));

  m_slp_server->FindService(
      scopes,
      request->service(),
      NewSingleCallback(this,
                        &SLPServiceImpl::FindServiceHandler,
                        response,
                        done));
}


/**
 * Register service request.
 */
void SLPDaemon::SLPServiceImpl::RegisterService(
    ::google::protobuf::RpcController*,
    const ola::slp::proto::ServiceRegistration* request,
    ola::slp::proto::ServiceAck* response,
    ::google::protobuf::Closure* done) {
  OLA_INFO << "Recv RegisterService request";

  set<string> scopes;
  for (int i = 0; i < request->scope_size(); ++i)
    scopes.insert(request->scope(i));

  m_slp_server->RegisterService(
      scopes,
      request->service(),
      request->lifetime(),
      NewSingleCallback(this, &SLPServiceImpl::AckHandler, response, done));
}


/**
 * De-Register service request.
 */
void SLPDaemon::SLPServiceImpl::DeRegisterService(
    ::google::protobuf::RpcController*,
    const ola::slp::proto::ServiceDeRegistration* request,
    ola::slp::proto::ServiceAck* response,
    ::google::protobuf::Closure* done) {
  OLA_INFO << "Recv DeRegisterService request";

  set<string> scopes;
  for (int i = 0; i < request->scope_size(); ++i)
    scopes.insert(request->scope(i));

  m_slp_server->DeRegisterService(
      scopes,
      request->service(),
      NewSingleCallback(this, &SLPServiceImpl::AckHandler, response, done));
}


/**
 * Called when FindService completes.
 */
void SLPDaemon::SLPServiceImpl::FindServiceHandler(
    ola::slp::proto::ServiceReply* response,
    ::google::protobuf::Closure* done,
    const URLEntries &services) {
  URLEntries::const_iterator iter = services.begin();
  for (; iter != services.end(); ++iter) {
    ola::slp::proto::Service *service = response->add_service();
    service->set_service_name(iter->URL());
    service->set_lifetime(iter->Lifetime());
  }
  done->Run();
}


/**
 * A RPC callback that accepts an SLP error code and replies with a ServiceAck.
 */
void SLPDaemon::SLPServiceImpl::AckHandler(
    ola::slp::proto::ServiceAck* response,
    ::google::protobuf::Closure* done,
    unsigned int error_code) {
  response->set_error_code(error_code);
  done->Run();
}
}  // slp
}  // ola
