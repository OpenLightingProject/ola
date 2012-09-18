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
 * SLPServer.cpp
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
#include <ola/io/MemoryBuffer.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/Socket.h>
#include <ola/network/SocketAddress.h>
#include <ola/network/TCPSocketFactory.h>

#ifdef HAVE_LIBMICROHTTPD
#include <ola/http/HTTPServer.h>
#include <ola/http/OlaHTTPServer.h>
#endif

#include <algorithm>
#include <string>
#include <vector>
#include <set>

#include "common/rpc/StreamRpcChannel.h"
#include "tools/slp/SLPPacketBuilder.h"
#include "tools/slp/SLPPacketParser.h"
#include "tools/slp/SLPServer.h"
#include "tools/slp/SLPServiceImpl.h"
#include "tools/slp/SLPStore.h"

namespace ola {
namespace slp {

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::ToUpper;
using ola::io::BigEndianInputStream;
using ola::io::BigEndianOutputStream;
using ola::io::MemoryBuffer;
using ola::network::HostToNetwork;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::NetworkToHost;
using ola::network::TCPAcceptingSocket;
using ola::network::TCPSocket;
using ola::network::UDPSocket;
using ola::rpc::StreamRpcChannel;
using std::auto_ptr;
using std::string;
using std::vector;


const char SLPServer::CONFIG_DA_BEAT_VAR[] = "slp-config-da-beat";
const char SLPServer::DA_ENABLED_VAR[] = "slp-da-enabled";
const char SLPServer::DA_SERVICE[] = "service:directory-agent";
const char SLPServer::DEFAULT_SCOPE[] = "DEFAULT";
const char SLPServer::SCOPE_LIST_VAR[] = "scope-list";
const char SLPServer::SLP_PORT_VAR[] = "slp-port";
const uint16_t SLPServer::DEFAULT_SLP_HTTP_PORT = 9012;
const uint16_t SLPServer::DEFAULT_SLP_PORT = 427;
const uint16_t SLPServer::DEFAULT_SLP_RPC_PORT = 9011;

void StdinHandler::HandleCharacter(char c) {
  m_slp_server->Input(c);
}


/**
 * Setup a new SLP server.
 * @param socket the UDP Socket to use for SLP messages.
 * @param options the SLP Server options.
 */
SLPServer::SLPServer(ola::network::UDPSocket *udp_socket,
                     ola::network::TCPAcceptingSocket *tcp_socket,
                     const SLPServerOptions &options,
                     ola::ExportMap *export_map)
    : m_enable_da(options.enable_da),
      m_config_da_beat(options.config_da_beat),
      m_en_lang(reinterpret_cast<const char*>(EN_LANGUAGE_TAG),
                sizeof(EN_LANGUAGE_TAG)),
      m_iface_address(options.ip_address),
      m_ss(export_map, &m_clock),
      m_stdin_handler(&m_ss, this),
      m_rpc_port(options.rpc_port),
      m_rpc_socket_factory(NewCallback(this, &SLPServer::NewTCPConnection)),
      m_rpc_accept_socket(&m_rpc_socket_factory),
      m_service_impl(new SLPServiceImpl(NULL)),
      m_udp_socket(udp_socket),
      m_slp_accept_socket(tcp_socket),
      m_udp_sender(m_udp_socket),
      m_export_map(export_map) {
  m_multicast_endpoint.reset(
      new IPV4SocketAddress(
        IPV4Address(HostToNetwork(SLP_MULTICAST_ADDRESS)), DEFAULT_SLP_PORT));

  ToLower(&m_en_lang);

#ifdef HAVE_LIBMICROHTTPD
  if (options.enable_http) {
    ola::http::HTTPServer::HTTPServerOptions http_options;
    http_options.port = options.http_port;

    m_http_server.reset(
        new ola::http::OlaHTTPServer(http_options, m_export_map));
  }
#endif

  if (options.scopes.empty())
    m_scope_list.insert(ScopedSLPStore::CanonicalScope(DEFAULT_SCOPE));

  std::transform(options.scopes.begin(),
                 options.scopes.end(),
                 inserter(m_scope_list, m_scope_list.begin()),
                 ScopedSLPStore::CanonicalScope);

  export_map->GetIntegerVar(CONFIG_DA_BEAT_VAR)->Set(options.config_da_beat);
  export_map->GetBoolVar(DA_ENABLED_VAR)->Set(options.enable_da);
  string joined_scopes = ola::StringJoin(",", m_scope_list);
  export_map->GetStringVar(SCOPE_LIST_VAR)->Set(joined_scopes);
}


SLPServer::~SLPServer() {
  m_udp_socket->Close();
  m_rpc_accept_socket.Close();
}


/**
 * Init the server
 */
bool SLPServer::Init() {
  OLA_INFO << "Interface address is " << m_iface_address;

  // setup the accepting TCP socket
  if (!m_rpc_accept_socket.Listen(
        IPV4SocketAddress(IPV4Address::Loopback(), m_rpc_port))) {
    return false;
  }

  m_ss.AddReadDescriptor(&m_rpc_accept_socket);

  if (!m_udp_socket->SetMulticastInterface(m_iface_address)) {
    m_rpc_accept_socket.Close();
    return false;
  }

  // join the multicast group
  if (!m_udp_socket->JoinMulticast(m_iface_address,
                                   m_multicast_endpoint->Host())) {
    m_rpc_accept_socket.Close();
    return false;
  }

  m_udp_socket->SetOnData(NewCallback(this, &SLPServer::UDPData));
  m_ss.AddReadDescriptor(m_udp_socket);

#ifdef HAVE_LIBMICROHTTPD
  if (m_http_server.get())
    m_http_server->Init();
#endif

  if (m_enable_da) {
    ola::Clock clock;
    clock.CurrentTime(&m_boot_time);

    // setup the DA beat timer
    m_ss.RegisterRepeatingTimeout(
        m_config_da_beat * 1000,
        NewCallback(this, &SLPServer::SendDABeat));
    SendDABeat();
  } else {
    // Send DA Locate
    SendFindDAService();
  }
  return true;
}


void SLPServer::Run() {
#ifdef HAVE_LIBMICROHTTPD
  if (m_http_server.get())
    m_http_server->Start();
#endif
  m_ss.Run();
}


void SLPServer::Stop() {
#ifdef HAVE_LIBMICROHTTPD
  if (m_http_server.get())
    m_http_server->Stop();
#endif
  m_ss.Terminate();
}


/**
 * Bulk load a set of URL Entries
 */
void SLPServer::BulkLoad(const string &scope,
                         const string &service,
                         const URLEntries &entries) {
  TimeStamp now;
  m_clock.CurrentTime(&now);
  string canonical_scope = scope;
  ToUpper(&canonical_scope);
  set<string>::iterator iter = m_scope_list.find(canonical_scope);
  if (iter == m_scope_list.end()) {
    OLA_WARN << "Ignoring registration for " << scope <<
      " since it's not configured";
    return;
  }
  SLPStore *store = m_service_store.LookupOrCreate(canonical_scope);
  store->BulkInsert(now, service, entries);
}


/*
 * Called when there is data on stdin.
 */
void SLPServer::Input(char c) {
  switch (c) {
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
void SLPServer::DumpStore() {
  set<string>::iterator iter = m_scope_list.begin();
  for (; iter != m_scope_list.end(); ++iter) {
    SLPStore *store = m_service_store.Lookup(*iter);
    if (!store)
      continue;
    OLA_INFO << "Scope: " << *iter;
    store->Dump(*(m_ss.WakeUpTime()));
  }
}


/**
 * Called when RPC client connects.
 */
void SLPServer::NewTCPConnection(TCPSocket *socket) {
  IPV4Address peer_address;
  uint16_t port;
  socket->GetPeer(&peer_address, &port);
  OLA_INFO << "New connection from " << peer_address << ":" << port;

  StreamRpcChannel *channel = new StreamRpcChannel(m_service_impl.get(), socket,
                                                   m_export_map);
  socket->SetOnClose(
      NewSingleCallback(this, &SLPServer::RPCSocketClosed, socket));

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
void SLPServer::RPCSocketClosed(TCPSocket *socket) {
  OLA_INFO << "RPC Socket closed";
  (void) socket;
}


/**
 * Called when there is data on the UDP socket
 */
void SLPServer::UDPData() {
  ssize_t packet_size = 1500;
  uint8_t packet[packet_size];
  ola::network::IPV4Address source_ip;
  uint16_t port;

  if (!m_udp_socket->RecvFrom(reinterpret_cast<uint8_t*>(&packet),
                              &packet_size, source_ip, port))
    return;
  IPV4SocketAddress source(source_ip, port);

  OLA_INFO << "Got " << packet_size << " UDP bytes from " << source;

  uint8_t function_id = m_packet_parser.DetermineFunctionID(packet,
                                                            packet_size);

  MemoryBuffer buffer(&packet[0], packet_size);
  BigEndianInputStream stream(&buffer);

  switch (function_id) {
    case 0:
      return;
    case SERVICE_REQUEST:
      HandleServiceRequest(&stream, source);
      break;
    case SERVICE_REPLY:
      HandleServiceReply(&stream, source);
      break;
    case SERVICE_REGISTRATION:
      HandleServiceRegistration(&stream, source);
      break;
    case SERVICE_ACKNOWLEDGE:
      HandleServiceAck(&stream, source);
      break;
    case DA_ADVERTISEMENT:
      HandleDAAdvert(&stream, source);
      break;
    case SERVICE_DEREGISTER:
    case ATTRIBUTE_REQUEST:
    case ATTRIBUTE_REPLY:
    case SERVICE_TYPE_REQUEST:
    case SERVICE_TYPE_REPLY:
    case SA_ADVERTISEMENT:
      OLA_INFO << "Unsupported SLP function-id: "
        << static_cast<int>(function_id);
      break;
    default:
      OLA_WARN << "Unknown SLP function-id: " << static_cast<int>(function_id);
      break;
  }
}


/**
 * Handle a Service Request packet.
 */
void SLPServer::HandleServiceRequest(BigEndianInputStream *stream,
                                     const IPV4SocketAddress &source) {
  OLA_INFO << "Got Service request from " << source;
  auto_ptr<const ServiceRequestPacket> request(
      m_packet_parser.UnpackServiceRequest(stream));
  if (!request.get())
    return;

  // if we're in the PR list don't do anything
  vector<IPV4Address>::const_iterator pr_iter = request->pr_list.begin();
  for (; pr_iter != request->pr_list.end(); ++pr_iter) {
    if (*pr_iter == m_iface_address) {
      OLA_INFO << m_iface_address <<
        " found in PR list, not responding to request";
      return;
    }
  }

  // check if any of the scopes match us
  if (!request->scope_list.empty()) {
    // if no match, return SCOPE_NOT_SUPPORTED if unicast

  } else {
    // no scope list,
  }

  if (!request->predicate.empty()) {
    OLA_WARN << "Recieved request with predicate, ignoring";
    return;
  }

  if (!request->spi.empty()) {
    OLA_WARN << "Recieved request with SPI";
    SendErrorIfUnicast(request.get(), source, AUTHENTICATION_UNKNOWN);
    return;
  }

  if (request->language != m_en_lang) {
    OLA_WARN << "Unsupported language " << request->language;
    SendErrorIfUnicast(request.get(), source, LANGUAGE_NOT_SUPPORTED);
    return;
  }

  if (request->service_type.empty()) {
    OLA_INFO << "Recieved SrvRqst with empty service-type from: " << source;
    SendErrorIfUnicast(request.get(), source, PARSE_ERROR);
    return;
  } else if (request->service_type == "service:directory-agent") {
    // if (m_enable_da)
      // SendDAAdvert
    return;
  } else if (request->service_type == "service:service-agent") {
    // if (!m_enable_da)
    //  SendSAAdvert
    return;
  }

  OLA_INFO << "Received SrvRqst for " << request->service_type;

  // handle it here
  OLA_INFO << "Unpacked service request";
  OLA_INFO << "xid: " << request->xid;
  OLA_INFO << "lang: " << request->language;
  OLA_INFO << "srv-type: " << request->service_type;
}


/**
 * Handle a Service Reply packet.
 */
void SLPServer::HandleServiceReply(BigEndianInputStream *stream,
                                   const IPV4SocketAddress &source) {
  OLA_INFO << "Got Service reply from " << source;
  const ServiceReplyPacket *srv_reply =
    m_packet_parser.UnpackServiceReply(stream);
  if (!srv_reply)
    return;

  OLA_INFO << "Unpacked service reply, error_code: " << srv_reply->error_code;
  delete srv_reply;
}


/**
 * Handle a Service Registration packet.
 */
void SLPServer::HandleServiceRegistration(BigEndianInputStream *stream,
                                          const IPV4SocketAddress &source) {
  OLA_INFO << "Got Service registration from " << source;
  const ServiceRegistrationPacket *srv_reg =
    m_packet_parser.UnpackServiceRegistration(stream);
  if (!srv_reg)
    return;

  OLA_INFO << "Unpacked service registration";
  delete srv_reg;
}


/**
 * Handle a Service Ack packet.
 */
void SLPServer::HandleServiceAck(BigEndianInputStream *stream,
                                 const IPV4SocketAddress &source) {
  OLA_INFO << "Got Service ack from " << source;
  const ServiceAckPacket *srv_ack =
    m_packet_parser.UnpackServiceAck(stream);
  if (!srv_ack)
    return;

  OLA_INFO << "Unpacked service ack";
  delete srv_ack;
}


/**
 * Handle a DAAdvert.
 */
void SLPServer::HandleDAAdvert(BigEndianInputStream *stream,
                               const IPV4SocketAddress &source) {
  OLA_INFO << "Got DAAdvert from " << source;
  const DAAdvertPacket *da_advert = m_packet_parser.UnpackDAAdvert(stream);
  if (!da_advert)
    return;

  OLA_INFO << "Unpacked DA Advert";
  delete da_advert;
}


/**
 * Perform some sanity checks on the SLP Packet
 * @returns true if this packet is ok, false otherwise.
bool SLPServer::PerformSanityChecks(const SLPPacket *packet,
                                    const IPV4SocketAddress &source) {



  return true;
}

 */

/**
 * Receive data on a TCP connection
void SLPServer::ReceiveTCPData(TCPSocket *socket) {
  uint8_t trash[512];
  unsigned int data_length;
  int ok = socket->Receive(trash, sizeof(trash), data_length);
  if (ok == 0)  {
    // OLA_INFO << "got " << data_length << " on a tcp connection";
    // ola::FormatData(&cout, trash, data_length);
    for (unsigned int i = 0; i < data_length; ++i) {
      if (trash[i] == 'g') {
        OLA_INFO << "Sending state";
        SendState(socket);
      }
    }
  }
}


 * Called when a socket is closed.
void SLPServer::SocketClosed(TCPSocket *socket) {
  auto_ptr<TCPSocket> socket_to_delete(socket);
  OLA_INFO << "closing TCP socket";
  m_ss.RemoveReadDescriptor(socket);

  TCPSocketList::iterator iter = m_slp_accept_sockets.begin();
  for (; iter != m_slp_accept_sockets.end(); ++iter) {
    if (*iter == socket)
      break;
  }

  if (iter == m_slp_accept_sockets.end()) {
    OLA_FATAL << "Unable to locate socket for " << socket;
    return;
  }

  m_slp_accept_sockets.erase(iter);
}


 * Send a join message to all connected clients
void SLPServer::SendJoinUpdateToClients(const IPV4Address &address,
                                        const UID &uid) {
  stringstream str;
  str << "Join: " << address << ", " << uid << endl;
  SendStringToClients(str.str());
}


 * Send a part message to all connected clients
void SLPServer::SendPartUpdateToClients(const IPV4Address &address,
                                        const UID &uid) {
  stringstream str;
  str << "Part: " << address << ", " << uid << endl;
  SendStringToClients(str.str());
}


 * Send a string to all clients
void SLPServer::SendStringToClients(const string &output) {
  TCPSocketList::iterator iter = m_slp_accept_sockets.begin();
  for (; iter != m_slp_accept_sockets.end(); ++iter) {
    (*iter)->Send(reinterpret_cast<const uint8_t*>(output.data()),
                  output.size());
  }
  OLA_INFO << "Sent \"" << output.substr(0, output.size() - 1) << "\" to " <<
    m_slp_accept_sockets.size() << " clients";
}


 * Send a full state message to a single client
void SLPServer::SendState(TCPSocket *socket) {
  NodeList::iterator iter = m_nodes.begin();
  for ( ; iter != m_nodes.end(); ++iter) {
    stringstream str;
    str << "Active: " << (*iter)->ip << ", " << (*iter)->uid << endl;
    string output = str.str();
    socket->Send(reinterpret_cast<const uint8_t*>(output.data()),
                 output.size());
  }
}

*/

/*
 * Send the periodic advertisment.
bool SLPServer::SendPeriodicAdvert() {
  OLA_INFO << "Sending advert";

  ola::plugin::e131::OutgoingUDPTransport transport(&m_outgoing_udp_transport,
                                                    m_multicast_address,
                                                    TLP_PORT);

  bool ok = m_root_sender.SendEmpty(TLP_REGISTRY_ADVERT_VECTOR, &transport);
  if (!ok)
    OLA_WARN << "Failed to send Advert";
  return true;
}


 * Create a node entry
void SLPServer::CreateNodeEntry(const IPV4Address &ip,
                                const UID &uid,
                                uint16_t lifetime) {
  const TimeStamp *now = m_ss.WakeUpTime();

  NodeList::iterator iter = m_nodes.begin();
  for (; iter != m_nodes.end(); ++iter) {
    if ((*iter)->ip == ip && (*iter)->uid == uid)
      break;
  }

  TimeStamp expiry = *now + TimeInterval(lifetime, 0);
  if (iter == m_nodes.end()) {
    OLA_INFO << "creating " << ip << ", " << uid << ", " << lifetime;
    NodeEntry *node = new NodeEntry(ip, uid, expiry);
    m_nodes.push_back(node);
    SendJoinUpdateToClients(ip, uid);
  } else {
    OLA_INFO << "updating " << ip << ", expires in " << lifetime << " seconds";
    (*iter)->expiry = expiry;
  }
}


 * Walk the list looking for stale entries
bool SLPServer::LookForStaleEntries() {
  OLA_INFO << "looking for stale entries";
  const TimeStamp now = *(m_ss.WakeUpTime());

  NodeList::iterator node_iter = m_nodes.begin();
  while (node_iter != m_nodes.end()) {
    if ((*node_iter)->expiry < now) {
      OLA_INFO << "Node has expired " << (*node_iter)->ip << ", " <<
        (*node_iter)->uid;
      SendPartUpdateToClients((*node_iter)->ip, (*node_iter)->uid);
      delete *node_iter;
      node_iter = m_nodes.erase(node_iter);
    } else {
      node_iter++;
    }
  }
  return true;
}
*/


void SLPServer::SendErrorIfUnicast(const ServiceRequestPacket *request,
                                   const IPV4SocketAddress &source,
                                   slp_error_code_t error_code) {
  if (request->Multicast())
    return;
  m_udp_sender.SendServiceReply(source, request->xid, error_code);
}


/**
 * Send a multicast DAAdvert packet
 */
bool SLPServer::SendDABeat() {
  OLA_INFO << "Sending Multicast DAAdvert";
  std::ostringstream str;
  str << DA_SERVICE << "://" << m_iface_address;
  m_udp_sender.SendDAAdvert(*m_multicast_endpoint, 0, 0, m_boot_time.Seconds(),
                            str.str(), m_scope_list);
  return true;
}


/**
 * Send a Service Request for 'directory-agent'
 */
bool SLPServer::SendFindDAService() {
  OLA_INFO << "Sending Multicast ServiceRequest for " << DA_SERVICE;
  m_udp_sender.SendServiceRequest(*m_multicast_endpoint, 0, m_da_pr_list,
                                  DA_SERVICE, m_scope_list);
  return true;
}
}  // slp
}  // ola
