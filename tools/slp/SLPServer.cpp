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

#include <algorithm>
#include <string>
#include <vector>
#include <set>
#include <sstream>

#include "tools/slp/SLPPacketBuilder.h"
#include "tools/slp/SLPPacketParser.h"
#include "tools/slp/SLPServer.h"
#include "tools/slp/SLPStore.h"
#include "tools/slp/SLPStrings.h"

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
using std::auto_ptr;
using std::ostringstream;
using std::string;
using std::vector;


const char SLPServer::CONFIG_DA_BEAT_VAR[] = "slp-config-da-beat";
const char SLPServer::DAADVERT[] = "DAAdvert";
const char SLPServer::DA_ENABLED_VAR[] = "slp-da-enabled";
const char SLPServer::DEFAULT_SCOPE[] = "DEFAULT";
const char SLPServer::DIRECTORY_AGENT_SERVICE[] = "service:directory-agent";
const char SLPServer::FINDSRVS_COUNT_VAR[] = "find-srvs";
const char SLPServer::FINDSRVS_EMPTY_COUNT_VAR[] = "find-srvs-empty-response";
const char SLPServer::SCOPE_LIST_VAR[] = "scope-list";
const char SLPServer::SERVICE_AGENT_SERVICE[] = "service:service-agent";
const char SLPServer::SLP_PORT_VAR[] = "slp-port";
const char SLPServer::SRVACK[] = "SrvAck";
const char SLPServer::SRVREG[] = "SrvReg";
const char SLPServer::SRVRPLY[] = "SrvRply";
const char SLPServer::SRVRQST[] = "SrvRqst";
// This counter tracks the number of packets received by type.
// This is incremented prior to packet checks.
const char SLPServer::UDP_RX_PACKET_BY_TYPE_VAR[] = "slp-udp-rx-packets";
// The total number of recieved SLP UDP packets
const char SLPServer::UDP_RX_TOTAL_VAR[] = "slp-udp-rx";
const char SLPServer::UDP_TX_PACKET_BY_TYPE_VAR[] = "slp-udp-tx-packets";

/**
 * Setup a new SLP server.
 * @param ss the SelectServer to use
 * @param udp_socket the socket to use for UDP SLP traffic
 * @param tcp_socket the TCP socket to listen for incoming TCP SLP connections.
 * @param export_map the ExportMap to use for exporting variables, may be NULL
 * @param options the SLP Server options.
 */
SLPServer::SLPServer(ola::io::SelectServerInterface *ss,
                     ola::network::UDPSocket *udp_socket,
                     ola::network::TCPAcceptingSocket *tcp_socket,
                     ola::ExportMap *export_map,
                     const SLPServerOptions &options)
    : m_enable_da(options.enable_da),
      m_config_da_beat(options.config_da_beat),
      m_en_lang(reinterpret_cast<const char*>(EN_LANGUAGE_TAG),
                sizeof(EN_LANGUAGE_TAG)),
      m_iface_address(options.ip_address),
      m_ss(ss),
      m_udp_socket(udp_socket),
      m_slp_accept_socket(tcp_socket),
      m_udp_sender(m_udp_socket),
      m_export_map(export_map) {
  m_multicast_endpoint.reset(
      new IPV4SocketAddress(
        IPV4Address(HostToNetwork(SLP_MULTICAST_ADDRESS)), DEFAULT_SLP_PORT));

  ToLower(&m_en_lang);

  if (options.scopes.empty())
    m_scope_list.insert(SLPGetCanonicalString(DEFAULT_SCOPE));

  std::transform(options.scopes.begin(),
                 options.scopes.end(),
                 inserter(m_scope_list, m_scope_list.begin()),
                 SLPGetCanonicalString);

  string joined_scopes = ola::StringJoin(",", m_scope_list);
  export_map->GetBoolVar(DA_ENABLED_VAR)->Set(options.enable_da);
  export_map->GetIntegerVar(CONFIG_DA_BEAT_VAR)->Set(options.config_da_beat);
  export_map->GetIntegerVar(FINDSRVS_COUNT_VAR);
  export_map->GetIntegerVar(FINDSRVS_EMPTY_COUNT_VAR);
  export_map->GetIntegerVar(UDP_RX_TOTAL_VAR);
  export_map->GetStringVar(SCOPE_LIST_VAR)->Set(joined_scopes);
  export_map->GetUIntMapVar(UDP_RX_PACKET_BY_TYPE_VAR, "type");
}


SLPServer::~SLPServer() {
  m_udp_socket->Close();
}


/**
 * Init the server
 */
bool SLPServer::Init() {
  OLA_INFO << "Interface address is " << m_iface_address;

  if (!m_udp_socket->SetMulticastInterface(m_iface_address)) {
    return false;
  }

  // join the multicast group
  if (!m_udp_socket->JoinMulticast(m_iface_address,
                                   m_multicast_endpoint->Host())) {
    return false;
  }

  m_udp_socket->SetOnData(NewCallback(this, &SLPServer::UDPData));
  m_ss->AddReadDescriptor(m_udp_socket);

  if (m_enable_da) {
    ola::Clock clock;
    clock.CurrentTime(&m_boot_time);

    // setup the DA beat timer
    m_ss->RegisterRepeatingTimeout(
        m_config_da_beat * 1000,
        NewCallback(this, &SLPServer::SendDABeat));
    SendDABeat();
  } else {
    // Send DA Locate
    SendFindDAService();
  }
  return true;
}


/**
 * Bulk load a set of URL Entries
 */
void SLPServer::BulkLoad(const string &scope,
                         const string &service,
                         const URLEntries &entries) {
  TimeStamp now;
  m_clock.CurrentTime(&now);
  string canonical_scope = SLPGetCanonicalString(scope);
  set<string>::iterator iter = m_scope_list.find(canonical_scope);
  if (iter == m_scope_list.end()) {
    OLA_WARN << "Ignoring registration for " << scope <<
      " since it's not configured";
    return;
  }
  SLPStore *store = m_service_store.LookupOrCreate(canonical_scope);
  store->BulkInsert(now, service, entries);
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
    store->Dump(*(m_ss->WakeUpTime()));
  }
}


/**
 * Locate a service
 * @param scopes the set of scopes to search
 * @param service the service name
 * @param cb the callback to run
 * TODO(simon): change this to execute the callback multiple times until all
 * results arrive.
 */
void SLPServer::FindService(const set<string> &scopes,
                            const string &service,
                            SingleUseCallback1<void, const URLEntries&> *cb) {
  (*m_export_map->GetIntegerVar(FINDSRVS_COUNT_VAR))++;
  URLEntries urls;
  if (m_enable_da) {
    // we're in DA mode, just return all the matching URLEntries we know about
    set<string> canonical_scopes;
    SLPReduceList(scopes, &canonical_scopes);
    if (SLPSetIntersect(canonical_scopes, m_scope_list)) {
      OLA_INFO << "Received SrvRqst for " << service;
      LocateServices(canonical_scopes, service, &urls);
    }
  } else {
    // UA mode, start finding the services
  }
  if (urls.empty())
    (*m_export_map->GetIntegerVar(FINDSRVS_EMPTY_COUNT_VAR))++;
  cb->Run(urls);
}


void SLPServer::RegisterService(const string &service,
                                uint16_t lifetime,
                                SingleUseCallback0<void> *cb) {
  (void) service;
  (void) lifetime;
  cb->Run();
}


void SLPServer::DeRegisterService(const string &service,
                                  SingleUseCallback0<void> *cb) {
  (void) service;
  cb->Run();
}


/**
 * Called when there is data on the UDP socket
 */
void SLPServer::UDPData() {
  // TODO(simon): make this a member when we're donet
  ssize_t packet_size = 1500;
  uint8_t packet[packet_size];
  ola::network::IPV4Address source_ip;
  uint16_t port;

  if (!m_udp_socket->RecvFrom(reinterpret_cast<uint8_t*>(&packet),
                              &packet_size, source_ip, port))
    return;
  IPV4SocketAddress source(source_ip, port);

  OLA_INFO << "Got " << packet_size << " UDP bytes from " << source;
  (*m_export_map->GetIntegerVar(UDP_RX_TOTAL_VAR))++;

  uint8_t function_id = m_packet_parser.DetermineFunctionID(packet,
                                                            packet_size);

  MemoryBuffer buffer(&packet[0], packet_size);
  BigEndianInputStream stream(&buffer);

  UIntMap *packet_by_type = m_export_map->GetUIntMapVar(
    UDP_RX_PACKET_BY_TYPE_VAR);

  switch (function_id) {
    case 0:
      return;
    case SERVICE_REQUEST:
      packet_by_type->Increment(SRVRQST);
      HandleServiceRequest(&stream, source);
      break;
    case SERVICE_REPLY:
      packet_by_type->Increment(SRVRPLY);
      HandleServiceReply(&stream, source);
      break;
    case SERVICE_REGISTRATION:
      packet_by_type->Increment(SRVREG);
      HandleServiceRegistration(&stream, source);
      break;
    case SERVICE_ACKNOWLEDGE:
      packet_by_type->Increment(SRVACK);
      HandleServiceAck(&stream, source);
      break;
    case DA_ADVERTISEMENT:
      packet_by_type->Increment(DAADVERT);
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

  // check service, MaybeSend[DS]AAdvert do their own scope checking
  if (request->service_type.empty()) {
    OLA_INFO << "Recieved SrvRqst with empty service-type from: " << source;
    SendErrorIfUnicast(request.get(), source, PARSE_ERROR);
    return;
  } else if (request->service_type == DIRECTORY_AGENT_SERVICE) {
    MaybeSendDAAdvert(request.get(), source);
    return;
  } else if (request->service_type == SERVICE_AGENT_SERVICE) {
    MaybeSendSAAdvert(request.get(), source);
    return;
  }

  // check scopes
  set<string> canonical_scopes;
  if (request->scope_list.empty()) {
    SendErrorIfUnicast(request.get(), source, SCOPE_NOT_SUPPORTED);
    return;
  }
  SLPReduceList(request->scope_list, &canonical_scopes);
  if (!SLPSetIntersect(canonical_scopes, m_scope_list)) {
    SendErrorIfUnicast(request.get(), source, SCOPE_NOT_SUPPORTED);
    return;
  }

  URLEntries urls;
  OLA_INFO << "Received SrvRqst for " << request->service_type;
  LocateServices(canonical_scopes, request->service_type, &urls);

  OLA_INFO << "sending SrvReply with " << urls.size() << " urls";
  m_udp_sender.SendServiceReply(source, request->xid, 0, urls);
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


void SLPServer::SendErrorIfUnicast(const ServiceRequestPacket *request,
                                   const IPV4SocketAddress &source,
                                   slp_error_code_t error_code) {
  if (request->Multicast())
    return;
  m_udp_sender.SendServiceReply(source, request->xid, error_code);
}


/**
 * Send a SAAdvert if allowed.
 */
void SLPServer::MaybeSendSAAdvert(const ServiceRequestPacket *request,
                                  const IPV4SocketAddress &source) {
  if (m_enable_da)
    return;  // no SAAdverts in DA mode

  // Section 11.2
  if (!(request->scope_list.empty() ||
        SLPScopesMatch(request->scope_list, m_scope_list))) {
    SendErrorIfUnicast(request, source, SCOPE_NOT_SUPPORTED);
    return;
  }

  ostringstream str;
  str << SERVICE_AGENT_SERVICE << "://" << m_iface_address;
  m_udp_sender.SendSAAdvert(source, request->xid, str.str(), m_scope_list);
}


/**
 * Send a DAAdvert if allows.
 */
void SLPServer::MaybeSendDAAdvert(const ServiceRequestPacket *request,
                                  const IPV4SocketAddress &source) {
  if (!m_enable_da)
    return;

  // Section 11.2
  if (!(request->scope_list.empty() ||
        SLPScopesMatch(request->scope_list, m_scope_list))) {
    SendErrorIfUnicast(request, source, SCOPE_NOT_SUPPORTED);
    return;
  }
  SendDAAdvert(source);
}


/**
 * Send a DAAdvert for this server
 */
void SLPServer::SendDAAdvert(const IPV4SocketAddress &dest) {
  OLA_INFO << "Sending DAAdvert";
  std::ostringstream str;
  str << DIRECTORY_AGENT_SERVICE << "://" << m_iface_address;
  m_udp_sender.SendDAAdvert(dest, 0, 0, m_boot_time.Seconds(),
                            str.str(), m_scope_list);
}

/**
 * Send a multicast DAAdvert packet
 */
bool SLPServer::SendDABeat() {
  SendDAAdvert(*m_multicast_endpoint);
  return true;
}


void SLPServer::LocateServices(const set<string> &scopes,
                                const string &service,
                                URLEntries *urls) {
  string service_type = SLPGetCanonicalString(service);
  SLPStripService(&service_type);

  for (set<string>::const_iterator iter = scopes.begin();
       iter != scopes.end(); ++iter) {
    SLPStore *store = m_service_store.Lookup(*iter);
    if (!store)
      continue;
    OLA_INFO << "Doing lookup for " << *iter << " - " << service_type;
    store->Lookup(*(m_ss->WakeUpTime()), service_type, urls);
  }
}


/**
 * Send a Service Request for 'directory-agent'
 */
bool SLPServer::SendFindDAService() {
  OLA_INFO << "Sending Multicast ServiceRequest for " <<
    DIRECTORY_AGENT_SERVICE;
  m_udp_sender.SendServiceRequest(*m_multicast_endpoint, 0, m_da_pr_list,
                                  DIRECTORY_AGENT_SERVICE, m_scope_list);
  return true;
}
}  // slp
}  // ola
