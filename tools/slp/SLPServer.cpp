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
#include <ola/io/SelectServer.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/Socket.h>
#include <ola/network/TCPSocketFactory.h>

#ifdef HAVE_LIBMICROHTTPD
#include <ola/http/HTTPServer.h>
#include <ola/http/OlaHTTPServer.h>
#endif

#include "common/rpc/StreamRpcChannel.h"
#include "tools/slp/SLPPacketBuilder.h"
#include "tools/slp/SLPServer.h"
#include "tools/slp/SLPServiceImpl.h"

#include <sstream>
#include <string>
#include <vector>


namespace ola {
namespace slp {


using ola::NewCallback;
using ola::NewSingleCallback;
using ola::network::HostToNetwork;
using ola::network::IPV4Address;
using ola::network::NetworkToHost;
using ola::network::TCPAcceptingSocket;
using ola::network::TCPSocket;
using ola::network::UDPSocket;
using ola::rpc::StreamRpcChannel;
using std::auto_ptr;
using std::string;
using std::stringstream;
using std::vector;


const uint16_t SLPServer::DEFAULT_SLP_PORT = 427;
const uint16_t SLPServer::DEFAULT_SLP_HTTP_PORT = 9012;
const char SLPServer::K_SLP_PORT_VAR[] = "slp-port";

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
      m_iface_address(options.ip_address),
      m_ss(export_map),
      m_stdin_handler(&m_ss, this),
      m_rpc_port(options.rpc_port),
      m_rpc_socket_factory(NewCallback(this, &SLPServer::NewTCPConnection)),
      m_rpc_accept_socket(&m_rpc_socket_factory),
      m_service_impl(new SLPServiceImpl(NULL)),
      m_udp_socket(udp_socket),
      m_slp_accept_socket(tcp_socket),
      m_export_map(export_map) {
  m_multicast_address = IPV4Address(
      HostToNetwork(239U << 24 |
                    255U << 16 |
                    255u << 8 |
                    253));

#ifdef HAVE_LIBMICROHTTPD
  if (options.enable_http) {
    ola::http::HTTPServer::HTTPServerOptions http_options;
    http_options.port = options.http_port;

    m_http_server.reset(
        new ola::http::OlaHTTPServer(http_options, m_export_map));
  }
#endif
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
  if (!m_rpc_accept_socket.Listen(m_iface_address, m_rpc_port)) {
    return false;
  }

  m_ss.AddReadDescriptor(&m_rpc_accept_socket);

  if (!m_udp_socket->SetMulticastInterface(m_iface_address)) {
    m_rpc_accept_socket.Close();
    return false;
  }

  // join the multicast group
  if (!m_udp_socket->JoinMulticast(m_iface_address, m_multicast_address)) {
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


/*
 * Called when there is data on stdin.
 */
void SLPServer::Input(char c) {
  switch (c) {
    case 'q':
      m_ss.Terminate();
      break;
    default:
      break;
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
  ola::network::IPV4Address source;
  uint16_t port;

  if (!m_udp_socket->RecvFrom(reinterpret_cast<uint8_t*>(&packet),
                              &packet_size, source, port))
    return;
}


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


 * Called when we get a UDP message
void SLPServer::UDPMessage(const IPV4Address &ip,
                           uint32_t vector,
                           const uint8_t *data,
                           unsigned int length) {
  OLA_INFO << "got udp message from " << ip << ", vector is " << vector;

  if (vector == TLP_REGISTER_VECTOR) {
    if (length != 8) {
      OLA_WARN << "data length wasn't 8!";
      return;
    }

    // extract the uid & lifetime
    UID client_uid(data);
    uint16_t lifetime;
    memcpy(reinterpret_cast<void*>(&lifetime), data + 6, 2);
    lifetime = NetworkToHost(lifetime);
    OLA_INFO << "UID " << client_uid << ", lifetime " << lifetime;

    CreateNodeEntry(ip, client_uid, lifetime);

    // send an ack
    ola::plugin::e131::OutgoingUDPTransport transport(
        &m_outgoing_udp_transport,
        ip,
        TLP_PORT);

    bool ok = m_root_sender.SendEmpty(TLP_REGISTER_ACK_VECTOR, &transport);
    if (!ok)
      OLA_WARN << "Failed to send ack";
  } else {
    OLA_INFO << "Got message with unknown vector " << vector;
  }
}
 */

bool SLPServer::SendDABeat() {
  IOQueue output;
  vector<string> scopes;
  scopes.push_back("default");

  std::ostringstream str;
  str << "service:directory-agent://" << m_iface_address;
  SLPPacketBuilder::BuildDAAdvert(&output,
                                  0, true, 0,
                                  m_boot_time.Seconds(),
                                  str.str(),
                                  scopes);
  OLA_INFO << "Sending Multicast DAAdvert";
  m_udp_socket->SendTo(&output, m_multicast_address, DEFAULT_SLP_PORT);
  return true;
}
}  // slp
}  // ola
