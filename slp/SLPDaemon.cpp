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
 * SLPDaemon.cpp
 * Copyright (C) 2012 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <ola/Callback.h>
#include <ola/Constants.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/io/BigEndianStream.h>
#include <ola/io/SelectServer.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/Socket.h>
#include <ola/network/SocketAddress.h>
#include <ola/stl/STLUtils.h>

#ifdef HAVE_LIBMICROHTTPD
#include <ola/http/HTTPServer.h>
#include <ola/http/OlaHTTPServer.h>
#endif

#include <iostream>
#include <string>
#include <set>
#include <vector>

#include "common/rpc/RpcChannel.h"
#include "common/rpc/RpcServer.h"
#include "slp/SLPDaemon.h"

namespace ola {
namespace slp {

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::TCPAcceptingSocket;
using ola::network::TCPSocket;
using ola::network::UDPSocket;
using ola::rpc::RpcChannel;
using ola::rpc::RpcServer;
using std::auto_ptr;
using std::string;


const uint16_t SLPDaemon::DEFAULT_SLP_HTTP_PORT = 9012;
const uint16_t SLPDaemon::DEFAULT_SLP_RPC_PORT = 9011;

/**
 * @brief Setup a new SLP server.
 * @param socket the UDP Socket to use for SLP messages.
 * @param options the SLP Server options.
 */
SLPDaemon::SLPDaemon(ola::network::UDPSocket *udp_socket,
                     ola::network::TCPAcceptingSocket *tcp_socket,
                     const SLPDaemonOptions &options,
                     ola::ExportMap *export_map)
    : m_ss(export_map, &m_clock),
      m_slp_server(&m_ss, udp_socket, tcp_socket, export_map, options),
      m_stdin_handler(&m_ss, NewCallback(this, &SLPDaemon::Input)),

      m_rpc_port(options.rpc_port),
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


SLPDaemon::~SLPDaemon() {}

/**
 * @brief Init the server
 */
bool SLPDaemon::Init() {
  if (!m_slp_server.Init())
    return false;

  // Initialize the RPC server.
  RpcServer::Options rpc_options;
  rpc_options.listen_port = m_rpc_port;
  rpc_options.export_map = m_export_map;

  auto_ptr<ola::rpc::RpcServer> rpc_server(
      new RpcServer(&m_ss, m_service_impl.get(), NULL, rpc_options));

  if (!rpc_server->Init()) {
    OLA_WARN << "Failed to init RPC server";
    return false;
  }

  m_rpc_server.reset(rpc_server.release());

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
 * @brief Bulk load a set of ServiceEntries
 */
bool SLPDaemon::BulkLoad(const ServiceEntries &services) {
  bool error = false;
  for (ServiceEntries::const_iterator iter = services.begin();
       iter != services.end(); ++iter) {
    error |= m_slp_server.RegisterService(*iter);
  }
  return !error;
}


/*
 * Called when there is data on stdin.
 */
void SLPDaemon::Input(char c) {
  switch (c) {
    case 'a':
      m_slp_server.TriggerActiveDADiscovery();
      break;
    case 'd':
      GetDirectoryAgents();
      break;
    case 'p':
      m_slp_server.DumpStore();
      break;
    case 'q':
      m_ss.Terminate();
      break;
    default:
      break;
  }
}


/**
 * Print a list of DAs
 */
void SLPDaemon::GetDirectoryAgents() {
  vector<DirectoryAgent> agents;
  m_slp_server.GetDirectoryAgents(&agents);
  for (vector<DirectoryAgent>::const_iterator iter = agents.begin();
       iter != agents.end(); ++iter)
    std::cout << *iter << std::endl;
}

//------------------------------------------------------------------------------
// SLPServiceImpl methods

/**
 * Find Service request.
 */
void SLPDaemon::SLPServiceImpl::FindService(
    ola::rpc::RpcController*,
    const ola::slp::proto::ServiceRequest* request,
    ola::slp::proto::ServiceReply* response,
    CompletionCallback* done) {
  OLA_INFO << "Recv FindService request";

  set<string> scopes;
  for (int i = 0; i < request->scope_size(); ++i)
    scopes.insert(request->scope(i));

  m_slp_server->FindService(
      scopes,
      request->service_type(),
      NewSingleCallback(this,
                        &SLPServiceImpl::FindServiceHandler,
                        response,
                        done));
}


/**
 * Register service request.
 */
void SLPDaemon::SLPServiceImpl::RegisterService(
    ola::rpc::RpcController*,
    const ola::slp::proto::ServiceRegistration* request,
    ola::slp::proto::ServiceAck* response,
    CompletionCallback* done) {
  OLA_INFO << "Recv RegisterService request";

  set<string> scopes;
  for (int i = 0; i < request->scope_size(); ++i)
    scopes.insert(request->scope(i));

  ServiceEntry service(ScopeSet(scopes), request->url(), request->lifetime());
  uint16_t error_code = m_slp_server->RegisterService(service);
  response->set_error_code(error_code);
  done->Run();
}


/**
 * De-Register service request.
 */
void SLPDaemon::SLPServiceImpl::DeRegisterService(
    ola::rpc::RpcController*,
    const ola::slp::proto::ServiceDeRegistration* request,
    ola::slp::proto::ServiceAck* response,
    CompletionCallback* done) {
  OLA_INFO << "Recv DeRegisterService request";

  set<string> scopes;
  for (int i = 0; i < request->scope_size(); ++i)
    scopes.insert(request->scope(i));

  // the lifetime can be anything for a de-register request
  ServiceEntry service(ScopeSet(scopes), request->url(), 0);
  uint16_t error_code = m_slp_server->DeRegisterService(service);
  response->set_error_code(error_code);
  done->Run();
}


/**
 * Get the server info
 */
void SLPDaemon::SLPServiceImpl::GetServerInfo(
    ola::rpc::RpcController*,
    const ola::slp::proto::ServerInfoRequest*,
    ola::slp::proto::ServerInfoReply* response,
    CompletionCallback* done) {
  OLA_INFO << "Recv GetServerInfo";

  response->set_da_enabled(m_slp_server->DAEnabled());
  response->set_port(m_slp_server->SLPPort());
  const ScopeSet &scopes = m_slp_server->ConfiguredScopes();
  ScopeSet::Iterator iter = scopes.begin();
  for (; iter != scopes.end(); ++iter) {
    response->add_scope(*iter);
  }
  done->Run();
}


/**
 * Called when FindService completes.
 */
void SLPDaemon::SLPServiceImpl::FindServiceHandler(
    ola::slp::proto::ServiceReply* response,
    CompletionCallback* done,
    const URLEntries &urls) {
  for (URLEntries::const_iterator iter = urls.begin();
       iter != urls.end(); ++iter) {
    ola::slp::proto::URLEntry *url_entry = response->add_url_entry();
    url_entry->set_url(iter->url());
    url_entry->set_lifetime(iter->lifetime());
  }
  done->Run();
}
}  // namespace slp
}  // namespace ola
