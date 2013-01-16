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
#include <ola/io/BigEndianStream.h>
#include <ola/io/MemoryBuffer.h>
#include <ola/io/SelectServer.h>
#include <ola/math/Random.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/Socket.h>
#include <ola/network/SocketAddress.h>
#include <ola/network/TCPSocketFactory.h>
#include <ola/stl/STLUtils.h>

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "tools/slp/SLPPacketBuilder.h"
#include "tools/slp/SLPPacketParser.h"
#include "tools/slp/SLPPendingOperations.h"
#include "tools/slp/SLPServer.h"
#include "tools/slp/SLPStore.h"
#include "tools/slp/SLPStrings.h"
#include "tools/slp/SLPUtil.h"
#include "tools/slp/ServerCommon.h"

namespace ola {
namespace slp {

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::ToUpper;
using ola::io::BigEndianInputStream;
using ola::io::BigEndianOutputStream;
using ola::io::MemoryBuffer;
using ola::math::Random;
using ola::network::HostToNetwork;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::NetworkToHost;
using ola::network::TCPAcceptingSocket;
using ola::network::TCPSocket;
using ola::network::UDPSocketInterface;
using std::auto_ptr;
using std::ostringstream;
using std::string;
using std::vector;


const char SLPServer::DAADVERT[] = "DAAdvert";
const char SLPServer::DEREGSRVS_ERROR_COUNT_VAR[] = "slp-dereg-srv-errors";
const char SLPServer::FINDSRVS_EMPTY_COUNT_VAR[] =
  "slp-find-srvs-empty-response";
const char SLPServer::METHOD_CALLS_VAR[] = "slp-server-methods";
const char SLPServer::METHOD_DEREG_SERVICE[] = "DeRegisterService";
const char SLPServer::METHOD_FIND_SERVICE[] = "FindService";
const char SLPServer::METHOD_REG_SERVICE[] = "RegisterService";
const char SLPServer::REGSRVS_ERROR_COUNT_VAR[] = "slp-reg-srv-errors";
const char SLPServer::SLP_PORT_VAR[] = "slp-port";
const char SLPServer::SRVACK[] = "SrvAck";
const char SLPServer::SRVDEREG[] = "SrvDeReg";
const char SLPServer::SRVREG[] = "SrvReg";
const char SLPServer::SRVRPLY[] = "SrvRply";
const char SLPServer::SRVRQST[] = "SrvRqst";
const char SLPServer::SRVTYPERQST[] = "SrvTypeRqst";
const char SLPServer::UNKNOWN[] = "Unknown";
const char SLPServer::UNSUPPORTED[] = "Unsupported";
// This counter tracks the number of packets received by type.
// This is incremented prior to packet checks.
const char SLPServer::UDP_RX_PACKET_BY_TYPE_VAR[] = "slp-udp-rx-packets";
// The total number of recieved SLP UDP packets
const char SLPServer::UDP_RX_TOTAL_VAR[] = "slp-udp-rx";
const char SLPServer::UDP_TX_PACKET_BY_TYPE_VAR[] = "slp-udp-tx-packets";


SLPServer::UnicastOperationDeleter::~UnicastOperationDeleter() {
  if (server && op)
    server->CancelPendingDAOperationsForServiceAndDA(op->service.url().url(),
                                                     op->da_url);
}

void SLPServer::UnicastOperationDeleter::Cancel() {
  op = NULL;
  server = NULL;
}


/*
 * Init the options to sensible defaults
 */
SLPServer::SLPServerOptions::SLPServerOptions()
    : clock(NULL),
      enable_da(true),
      slp_port(DEFAULT_SLP_PORT),
      config_da_find(CONFIG_DA_FIND),
      config_da_beat(CONFIG_DA_BEAT),
      config_mc_max(CONFIG_MC_MAX),
      config_retry(CONFIG_RETRY),
      config_retry_max(CONFIG_RETRY_MAX),
      config_start_wait(CONFIG_START_WAIT),
      config_reg_active_min(CONFIG_REG_ACTIVE_MIN),
      config_reg_active_max(CONFIG_REG_ACTIVE_MAX),
      initial_xid(Random(0, MAX_XID)),
      boot_time(0) {
}


/**
 * Setup a new SLP server.
 * @param ss the SelectServer to use
 * @param udp_socket the socket to use for UDP SLP traffic
 * @param tcp_socket the TCP socket to listen for incoming TCP SLP connections.
 * @param export_map the ExportMap to use for exporting variables, may be NULL
 * @param options the SLP Server options.
 */
SLPServer::SLPServer(ola::io::SelectServerInterface *ss,
                     ola::network::UDPSocketInterface *udp_socket,
                     ola::network::TCPAcceptingSocket *tcp_socket,
                     ola::ExportMap *export_map,
                     const SLPServerOptions &options)
    : m_config_da_beat(options.config_da_beat * ONE_THOUSAND),
      m_config_da_find(options.config_da_find * ONE_THOUSAND),
      m_config_mc_max(options.config_mc_max * ONE_THOUSAND),
      m_config_retry(options.config_retry * ONE_THOUSAND),
      m_config_retry_max(options.config_retry_max * ONE_THOUSAND),
      m_config_start_wait(options.config_start_wait * ONE_THOUSAND),
      m_config_reg_active_min(options.config_reg_active_min * ONE_THOUSAND),
      m_config_reg_active_max(options.config_reg_active_max * ONE_THOUSAND),
      m_enable_da(options.enable_da),
      m_slp_port(options.slp_port),
      m_en_lang(reinterpret_cast<const char*>(EN_LANGUAGE_TAG),
                sizeof(EN_LANGUAGE_TAG)),
      m_iface_address(options.ip_address),
      m_multicast_endpoint(IPV4SocketAddress(
            IPV4Address(HostToNetwork(SLP_MULTICAST_ADDRESS)), m_slp_port)),
      m_ss(ss),
      m_clock(options.clock),
      m_da_beat_timer(ola::thread::INVALID_TIMEOUT),
      m_store_cleaner_timer(ola::thread::INVALID_TIMEOUT),
      m_active_da_discovery_timer(ola::thread::INVALID_TIMEOUT),
      m_udp_socket(udp_socket),
      m_slp_accept_socket(tcp_socket),
      m_udp_sender(m_udp_socket),
      m_configured_scopes(options.scopes),
      m_xid_allocator(options.initial_xid),
      m_export_map(export_map) {
  ToLower(&m_en_lang);

  if (m_configured_scopes.empty())
    m_configured_scopes = ScopeSet(DEFAULT_SLP_SCOPE);

  if (export_map) {
    export_map->GetBoolVar("slp-da-enabled")->Set(options.enable_da);
    export_map->GetIntegerVar("slp-config-da-beat")->Set(
        options.config_da_beat);
    export_map->GetIntegerVar("slp-config-da-find")->Set(
        options.config_da_find);
    export_map->GetIntegerVar("slp-config-mc-max")->Set(options.config_mc_max);
    export_map->GetIntegerVar("slp-config-retry")->Set(options.config_retry);
    export_map->GetIntegerVar("slp-config-retry-max")->Set(
      options.config_retry_max);
    export_map->GetIntegerVar("slp-config-start_wait")->Set(
      options.config_start_wait);
    export_map->GetIntegerVar("slp-port")->Set(options.slp_port);
    export_map->GetIntegerVar(FINDSRVS_EMPTY_COUNT_VAR);
    export_map->GetIntegerVar(UDP_RX_TOTAL_VAR);
    export_map->GetStringVar("slp-scope-list")->Set(
        m_configured_scopes.ToString());
    export_map->GetUIntMapVar(UDP_RX_PACKET_BY_TYPE_VAR, "type");
    export_map->GetUIntMapVar(METHOD_CALLS_VAR, "method");
  }

  if (options.boot_time) {
    m_boot_time += TimeInterval(options.boot_time, 0);
  }
}


SLPServer::~SLPServer() {
  if (m_enable_da) {
    // send a DAAdvert with a boot time of 0 to let everyone know we're going
    // down
    SendDAAdvert(m_multicast_endpoint, 0, 0);
  }

  m_ss->RemoveTimeout(m_da_beat_timer);
  m_ss->RemoveTimeout(m_store_cleaner_timer);
  m_ss->RemoveTimeout(m_active_da_discovery_timer);

  if (m_outstanding_da_discovery.get()) {
    m_ss->RemoveTimeout(m_outstanding_da_discovery->timer_id);
  }

  // delete any pending registration operations
  for (PendingOperationsByURL::iterator iter = m_pending_ops.begin();
       iter != m_pending_ops.end(); ++iter) {
    FreePendingDAOperation(iter->second);
  }

  m_udp_socket->Close();
  OLA_INFO << "Size of m_pending_acks is " << m_pending_acks.size();
  OLA_INFO << "Size of m_pending_replies is " << m_pending_replies.size();
  STLDeleteValues(m_pending_acks);
}


/**
 * Init the server
 */
bool SLPServer::Init() {
  OLA_INFO << "SLP Interface address is " << m_iface_address;

  if (!m_udp_socket->SetMulticastInterface(m_iface_address)) {
    return false;
  }

  // join the multicast group
  if (!m_udp_socket->JoinMulticast(m_iface_address,
                                   m_multicast_endpoint.Host())) {
    return false;
  }

  m_udp_socket->SetOnData(NewCallback(this, &SLPServer::UDPData));
  m_ss->AddReadDescriptor(m_udp_socket);

  // Setup a timeout to clean up the store
  m_store_cleaner_timer = m_ss->RegisterRepeatingTimeout(
      30 * 1000,
      NewCallback(this, &SLPServer::CleanSLPStore));

  // Register a callback to find out about new DAs
  m_da_tracker.AddNewDACallback(NewCallback(this, &SLPServer::NewDACallback));

  if (m_enable_da) {
    if (m_boot_time.Seconds() == 0)
      GetCurrentTime(&m_boot_time);

    // setup the DA beat timer
    m_da_beat_timer = m_ss->RegisterRepeatingTimeout(
        m_config_da_beat,
        NewCallback(this, &SLPServer::SendDABeat));
    SendDABeat();
  }

  // schedule a SrvRqst for the directory agent
  // Even DAs need to know about other DAs, since they may also be UAs or SAs
  m_active_da_discovery_timer = m_ss->RegisterSingleTimeout(
      Random(0, m_config_start_wait),
      NewSingleCallback(this, &SLPServer::StartActiveDADiscovery));
  return true;
}


/**
 * Dump out the contents of the SLP store.
 */
void SLPServer::DumpStore() {
  m_service_store.Dump(*(m_ss->WakeUpTime()));
}


/**
 * Get a list of known DAs
 */
void SLPServer::GetDirectoryAgents(vector<DirectoryAgent> *output) {
  m_da_tracker.GetDirectoryAgents(output);
}


/**
 * Manually trigger active DA discovery.
 */
void SLPServer::TriggerActiveDADiscovery() {
  StartActiveDADiscovery();
}


/**
 * Locate a service
 * @param scopes the set of scopes to search
 * @param service_type the type of service to locate
 * @param cb the callback to run
 * TODO(simon): change this to execute the callback multiple times until all
 * results arrive.
 */
void SLPServer::FindService(
    const set<string> &scopes,
    const string &service_type,
    BaseCallback1<void, const URLEntries&> *cb) {
  IncrementMethodVar(METHOD_FIND_SERVICE);
  URLEntries urls;
  ScopeSet scope_set(scopes);
  OLA_INFO << "FindService(" << scope_set << ", " << service_type << ")";

  if (m_enable_da) {
    // if we're a DA handle all those scopes first
    ScopeSet da_scopes = scope_set.DifferenceUpdate(m_configured_scopes);
    if (!da_scopes.empty())
      m_service_store.Lookup(*(m_ss->WakeUpTime()), da_scopes, service_type,
                             &urls);
  }

  if (scope_set.empty()) {
    // all scopes were handled by our local DA
    if (urls.empty() && m_export_map)
      (*m_export_map->GetIntegerVar(FINDSRVS_EMPTY_COUNT_VAR))++;
    cb->Run(urls);
    return;
  }

  PendingSrvRqst *srv_request_state = new PendingSrvRqst(service_type,
                                                         scope_set, cb);
  srv_request_state->urls = urls;
  FindServiceInScopes(srv_request_state, scope_set);
}


/**
 * Register a service
 * @param service the ServiceEntry to register
 * @returns an SLP error code
 */
uint16_t SLPServer::RegisterService(const ServiceEntry &new_service) {
  IncrementMethodVar(METHOD_REG_SERVICE);
  ServiceEntry service(new_service);
  service.set_local(true);

  uint16_t error_code;
  if (!service.url().lifetime()) {
    OLA_WARN << "Attempt to register " << service << " with a lifetime of 0";
    error_code = INVALID_REGISTRATION;
  } else {
    error_code = InternalRegisterService(service);
  }
  if (error_code && m_export_map)
    (*m_export_map->GetIntegerVar(REGSRVS_ERROR_COUNT_VAR))++;
  return error_code;
}


/**
 * DeRegister a service
 * @param service the ServiceEntry to de-register
 * @returns an SLP error code
 */
uint16_t SLPServer::DeRegisterService(const ServiceEntry &service) {
  IncrementMethodVar(METHOD_DEREG_SERVICE);
  uint16_t error_code = InternalDeRegisterService(service);
  if (error_code && m_export_map)
    (*m_export_map->GetIntegerVar(DEREGSRVS_ERROR_COUNT_VAR))++;
  return error_code;
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

  OLA_DEBUG << "Got " << packet_size << " UDP bytes from " << source;
  if (m_export_map)
    (*m_export_map->GetIntegerVar(UDP_RX_TOTAL_VAR))++;

  uint8_t function_id = SLPPacketParser::DetermineFunctionID(packet,
                                                             packet_size);

  MemoryBuffer buffer(&packet[0], packet_size);
  BigEndianInputStream stream(&buffer);

  switch (function_id) {
    case 0:
      return;
    case SERVICE_REQUEST:
      IncrementPacketVar(SRVRQST);
      HandleServiceRequest(&stream, source);
      break;
    case SERVICE_REPLY:
      IncrementPacketVar(SRVRPLY);
      HandleServiceReply(&stream, source);
      break;
    case SERVICE_REGISTRATION:
      IncrementPacketVar(SRVREG);
      HandleServiceRegistration(&stream, source);
      break;
    case SERVICE_ACKNOWLEDGE:
      IncrementPacketVar(SRVACK);
      HandleServiceAck(&stream, source);
      break;
    case DA_ADVERTISEMENT:
      IncrementPacketVar(DAADVERT);
      HandleDAAdvert(&stream, source);
      break;
    case SERVICE_TYPE_REQUEST:
      IncrementPacketVar(SRVTYPERQST);
      HandleServiceTypeRequest(&stream, source);
      break;
    case SERVICE_DEREGISTER:
      IncrementPacketVar(SRVDEREG);
      HandleServiceDeRegister(&stream, source);
      break;
    case ATTRIBUTE_REQUEST:
    case ATTRIBUTE_REPLY:
    case SERVICE_TYPE_REPLY:
    case SA_ADVERTISEMENT:
      IncrementPacketVar(UNSUPPORTED);
      OLA_INFO << "Unsupported SLP function-id: "
        << static_cast<int>(function_id);
      break;
    default:
      IncrementPacketVar(UNKNOWN);
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
      SLPPacketParser::UnpackServiceRequest(stream));
  if (!request.get())
    return;

  // if we're in the PR list don't do anything
  if (InPRList(request->pr_list)) {
    OLA_INFO << m_iface_address <<
      " found in PR list, not responding to request";
    return;
  }

  if (!request->predicate.empty()) {
    OLA_WARN << "Recieved request with predicate, ignoring";
    return;
  }

  if (!request->spi.empty()) {
    OLA_WARN << "Recieved request with SPI";
    SendErrorIfUnicast(request.get(), SERVICE_REPLY, source,
                       AUTHENTICATION_UNKNOWN);
    return;
  }

  if (request->language != m_en_lang) {
    OLA_WARN << "Unsupported language " << request->language;
    SendErrorIfUnicast(request.get(), SERVICE_REPLY, source,
                       LANGUAGE_NOT_SUPPORTED);
    return;
  }

  OLA_INFO << "SrvRqst for '" << request->service_type << "'";
  // check service, MaybeSend[DS]AAdvert do their own scope checking
  if (request->service_type.empty()) {
    OLA_INFO << "Recieved SrvRqst with empty service-type from: " << source;
    SendErrorIfUnicast(request.get(), SERVICE_REPLY, source, PARSE_ERROR);
    return;
  } else if (m_enable_da && request->service_type == DIRECTORY_AGENT_SERVICE) {
    MaybeSendDAAdvert(request.get(), source);
    return;
  } else if (!m_enable_da && request->service_type == SERVICE_AGENT_SERVICE) {
    MaybeSendSAAdvert(request.get(), source);
    return;
  }

  // check scopes
  if (request->scope_list.empty()) {
    SendErrorIfUnicast(request.get(), SERVICE_REPLY, source,
                       SCOPE_NOT_SUPPORTED);
    return;
  }
  ScopeSet scope_set(request->scope_list);

  if (!scope_set.Intersects(m_configured_scopes)) {
    SendErrorIfUnicast(request.get(), SERVICE_REPLY, source,
                       SCOPE_NOT_SUPPORTED);
    return;
  }

  URLEntries urls;
  OLA_INFO << "Received SrvRqst for " << request->service_type;
  m_service_store.Lookup(*(m_ss->WakeUpTime()), scope_set,
                         request->service_type, &urls);

  OLA_INFO << "sending SrvReply with " << urls.size() << " urls";
  if (urls.empty() && request->Multicast())
    return;
  m_udp_sender.SendServiceReply(source, request->xid, 0, urls);
}


/**
 * Handle a Service Reply packet.
 */
void SLPServer::HandleServiceReply(BigEndianInputStream *stream,
                                   const IPV4SocketAddress &source) {
  OLA_INFO << "Got Service reply from " << source;
  auto_ptr<const ServiceReplyPacket> srv_reply(
    SLPPacketParser::UnpackServiceReply(stream));
  if (!srv_reply.get())
    return;

  PendingReplyMap::iterator iter = m_pending_replies.find(srv_reply->xid);
  if (iter == m_pending_replies.end()) {
    OLA_INFO << "Can't locate a matching SrvRqst for xid " << srv_reply->xid;
  } else {
    // we don't erase the iter, that's left up to the callback to do.
    iter->second->Run(source.Host(), srv_reply->error_code,
                      srv_reply->url_entries);
  }
}


/**
 * Handle a Service Registration packet, only DAs support this.
 */
void SLPServer::HandleServiceRegistration(BigEndianInputStream *stream,
                                          const IPV4SocketAddress &source) {
  OLA_INFO << "Got Service registration from " << source;
  auto_ptr<const ServiceRegistrationPacket> srv_reg(
    SLPPacketParser::UnpackServiceRegistration(stream));
  if (!srv_reg.get())
    return;

  ScopeSet scopes(srv_reg->scope_list);
  OLA_INFO << "Unpacked service registration for " << srv_reg->url
           << ", service-type " << srv_reg->service_type << ", with scopes "
           << scopes;

  if (!m_enable_da)
    return;

  if (srv_reg->url.lifetime() == 0) {
    m_udp_sender.SendServiceAck(source, srv_reg->xid, INVALID_REGISTRATION);
    return;
  }

  if (!m_configured_scopes.IsSuperSet(scopes)) {
    m_udp_sender.SendServiceAck(source, srv_reg->xid, SCOPE_NOT_SUPPORTED);
    return;
  }

  ServiceEntry service(scopes, srv_reg->service_type, srv_reg->url.url(),
                       srv_reg->url.lifetime());
  slp_error_code_t error_code = m_service_store.Insert(
      *(m_ss->WakeUpTime()), service, srv_reg->Fresh());

  m_udp_sender.SendServiceAck(source, srv_reg->xid, error_code);
}


/**
 * Handle a Service De-Registration packet, only DAs support this.
 */
void SLPServer::HandleServiceDeRegister(BigEndianInputStream *stream,
                                        const IPV4SocketAddress &source) {
  OLA_INFO << "Got Service de-registration from " << source;
  auto_ptr<const ServiceDeRegistrationPacket> srv_dereg(
    SLPPacketParser::UnpackServiceDeRegistration(stream));
  if (!srv_dereg.get())
    return;

  ScopeSet scopes(srv_dereg->scope_list);
  OLA_INFO << "Unpacked service de-registration for " << srv_dereg->url
           << ", scopes " << scopes;

  if (!m_enable_da)
    return;

  // lifetime can be anything for a dereg
  ServiceEntry service(scopes, srv_dereg->url.url(), 0);
  slp_error_code_t ret = m_service_store.Remove(service);
  m_udp_sender.SendServiceAck(source, srv_dereg->xid, ret);
}


/**
 * Handle a Service Ack packet.
 */
void SLPServer::HandleServiceAck(BigEndianInputStream *stream,
                                 const IPV4SocketAddress &source) {
  auto_ptr<const ServiceAckPacket> srv_ack(
      SLPPacketParser::UnpackServiceAck(stream));
  if (!srv_ack.get())
    return;

  // See if this matches one of our pending transactions
  PendingAckMap::iterator iter = m_pending_acks.find(srv_ack->xid);
  if (iter == m_pending_acks.end()) {
    OLA_INFO << "Can't locate a matching request for xid " << srv_ack->xid;
    return;
  }

  OLA_INFO << "SrvAck[" << srv_ack->xid << "] from " << source
           << ", error code is " << srv_ack->error_code;
  AckCallback *cb = iter->second;
  m_pending_acks.erase(iter);
  cb->Run(srv_ack->error_code);
}


/**
 * Handle a DAAdvert.
 */
void SLPServer::HandleDAAdvert(BigEndianInputStream *stream,
                               const IPV4SocketAddress &source) {
  auto_ptr<const DAAdvertPacket> da_advert(
      SLPPacketParser::UnpackDAAdvert(stream));
  if (!da_advert.get()) {
    OLA_INFO << "Dropped DAAdvert from " << source << " due to parse error";
    return;
  }

  if (da_advert->error_code) {
    OLA_WARN << "DAAdvert(" << source << "), error "
             << da_advert->error_code << " ("
             << SLPErrorToString(da_advert->error_code) << ")";
    return;
  }

  OLA_INFO << "RX DAAdvert(" << source << "), xid " << da_advert->xid
    << ", scopes " << da_advert->scope_list << ", boot "
    << da_advert->boot_timestamp << ", " << da_advert->url;

  if (m_outstanding_da_discovery.get()) {
    // active discovery in progress
    m_outstanding_da_discovery->AddPR(source.Host());
  }
  m_da_tracker.NewDAAdvert(*da_advert, source);
}


/**
 * Handle a SrvTypeRqst.
 */
void SLPServer::HandleServiceTypeRequest(BigEndianInputStream *stream,
                                         const IPV4SocketAddress &source) {
  auto_ptr<const ServiceTypeRequestPacket> request(
      SLPPacketParser::UnpackServiceTypeRequest(stream));
  if (!request.get()) {
    OLA_INFO << "Dropped SrvTypeRqst from " << source << " due to parse error";
    return;
  }

  // If we're listed in the PR list ignore the request
  if (InPRList(request->pr_list)) {
    OLA_INFO << m_iface_address <<
      " found in PR list, not responding to request";
    return;
  }

  ScopeSet scopes(request->scope_list);

  if (!scopes.Intersects(m_configured_scopes)) {
    if (!request->Multicast())
      m_udp_sender.SendError(source, SERVICE_TYPE_REPLY, request->xid,
                             SCOPE_NOT_SUPPORTED);
    return;
  }
  OLA_INFO << "RX SrvTypeRqst(" << source << "), scopes " << scopes
           << ", naming auth '" << request->naming_authority << "'";

  vector<string> service_types;
  if (request->include_all) {
    m_service_store.GetAllServiceTypes(scopes, &service_types);
  } else {
    m_service_store.GetServiceTypesByNamingAuth(request->naming_authority,
                                                scopes, &service_types);
  }

  if (service_types.empty() && request->Multicast())
    return;

  sort(service_types.begin(), service_types.end());

  m_udp_sender.SendServiceTypeReply(source, request->xid, SLP_OK,
                                    service_types);
}


/**
 * Send an error response, only if this request was unicast
 * @param request the request that triggered the response
 * @param function_id the function-id to use in the response
 * @param destination the socket address to send the message to
 * @param error_code the error code to use
 */
void SLPServer::SendErrorIfUnicast(const ServiceRequestPacket *request,
                                   slp_function_id_t function_id,
                                   const IPV4SocketAddress &destination,
                                   slp_error_code_t error_code) {
  if (request->Multicast())
    return;
  // Per section 7, we can truncate the message if the error code is non-0
  // It turns out the truncated message is identicate to an SrvAck (who would
  // have thought!) so we reuse that method here
  m_udp_sender.SendError(destination, function_id, request->xid, error_code);
}


/**
 * Send a SAAdvert if allowed.
 */
void SLPServer::MaybeSendSAAdvert(const ServiceRequestPacket *request,
                                  const IPV4SocketAddress &source) {
  if (m_enable_da)
    return;  // no SAAdverts in DA mode

  // Section 11.2
  ScopeSet scopes(request->scope_list);
  if (!(scopes.empty() || scopes.Intersects(m_configured_scopes))) {
    SendErrorIfUnicast(request, SERVICE_REPLY, source, SCOPE_NOT_SUPPORTED);
    return;
  }

  ostringstream str;
  str << SERVICE_AGENT_SERVICE << "://" << m_iface_address;
  m_udp_sender.SendSAAdvert(source, request->xid, str.str(),
                            m_configured_scopes);
}


/**
 * Send a DAAdvert if allows.
 */
void SLPServer::MaybeSendDAAdvert(const ServiceRequestPacket *request,
                                  const IPV4SocketAddress &source) {
  if (!m_enable_da)
    return;

  // Section 11.2
  ScopeSet scopes(request->scope_list);
  if (!scopes.empty() && !scopes.Intersects(m_configured_scopes)) {
    OLA_INFO << "Scopes in SrvRqst " << DIRECTORY_AGENT_SERVICE << ": '"
             << scopes << "', don't match our scopes of '"
             << m_configured_scopes << "'";
    SendErrorIfUnicast(request, DA_ADVERTISEMENT, source, SCOPE_NOT_SUPPORTED);
    return;
  }
  SendDAAdvert(source, m_boot_time.Seconds(), request->xid);
}


/**
 * Send a DAAdvert for this server
 */
void SLPServer::SendDAAdvert(const IPV4SocketAddress &dest,
                             uint32_t boot_time,
                             xid_t xid) {
  OLA_INFO << "Sending DAAdvert to " << dest;
  std::ostringstream str;
  str << DIRECTORY_AGENT_SERVICE << "://" << m_iface_address;
  m_udp_sender.SendDAAdvert(dest, xid, 0, boot_time, str.str(),
                            m_configured_scopes);
}

/**
 * Send a multicast DAAdvert packet
 */
bool SLPServer::SendDABeat() {
  // unsolicited DAAdverts have a xid of 0
  SendDAAdvert(m_multicast_endpoint, m_boot_time.Seconds(), 0);
  return true;
}


// UA methods
//------------------------------------------------------------------------------

/**
 * For the given scopes, check if there are any DAs to use and if so, send
 * SrvRqst messages. For scopes without DAs start the multicast dance.
 */
void SLPServer::FindServiceInScopes(PendingSrvRqst *request,
                                    const ScopeSet &scopes) {
  vector<DirectoryAgent> das;
  m_da_tracker.GetMinimalCoveringList(scopes, &das);
  ScopeSet remaining_scopes = scopes;

  for (vector<DirectoryAgent>::iterator iter = das.begin(); iter != das.end();
       iter++) {
    ScopeSet this_das_scopes = remaining_scopes.DifferenceUpdate(
        iter->scopes());
    if (this_das_scopes.empty()) {
      OLA_WARN << "Scopes for " << *iter
               << " are empty, this is a bug in GetMinimalCoveringList";
      continue;
    }

    UnicastSrvRqstOperation *op = new UnicastSrvRqstOperation(
        m_xid_allocator.Next(), m_config_retry, iter->URL(),
        this_das_scopes, request);
    SendSrvRqstToDA(op, *iter);
  }

  if (remaining_scopes.empty())
    return;

  // fallback to multicast for the rest
  OLA_WARN << "We need to multicast for '" << remaining_scopes << "'";
  MulicastSrvRqstOperation *op = new MulicastSrvRqstOperation(
      m_xid_allocator.Next(), m_config_retry, remaining_scopes, request);
  op->timer_id = m_ss->RegisterSingleTimeout(
      op->retry_time(),
      NewSingleCallback(this, &SLPServer::RequestServiceMulticastTimeout, op));
  m_udp_sender.SendServiceRequest(m_multicast_endpoint, op->xid, op->pr_list,
                                  op->parent->service_type, op->scopes);
  OLA_INFO << "adding callback for " << op->xid;
  // note the multi-use callback
  pair<xid_t, SrvReplyCallback*> p(
    op->xid, NewCallback(this, &SLPServer::ReceivedSASrvReply, op));
  if (!m_pending_replies.insert(p).second)
    OLA_WARN << "Collision for xid " << op->xid
             << ", we're probably leaking memory!";
}


/**
 * Send the SrvRqst to a DA, schedule the timeout and add the rx callbacks.
 */
void SLPServer::SendSrvRqstToDA(UnicastSrvRqstOperation *op,
                                const DirectoryAgent &da,
                                bool expect_reused_xid) {
  op->da_busy = false;  // reset the busy flag
  op->timer_id = m_ss->RegisterSingleTimeout(
      op->retry_time(),
      NewSingleCallback(this, &SLPServer::RequestServiceDATimeout, op));
  m_udp_sender.SendServiceRequest(
      IPV4SocketAddress(da.IPAddress(), m_slp_port), op->xid,
                        op->parent->service_type, op->scopes);
  OLA_INFO << "adding callback for " << op->xid;

  PendingReplyMap::iterator iter = m_pending_replies.find(op->xid);
  if (iter == m_pending_replies.end()) {
    pair<xid_t, SrvReplyCallback*> p(
      op->xid, NewSingleCallback(this, &SLPServer::ReceivedDASrvReply, op));
    m_pending_replies.insert(p);
  } else if (!expect_reused_xid) {
    OLA_WARN << "Collision for xid " << op->xid
             << ", we're probably leaking memory!";
  }
}


/**
 * Called when we receive a reply to a SrvRqst from a DA.
 */
void SLPServer::ReceivedDASrvReply(UnicastSrvRqstOperation *op,
                                   const IPV4Address&,
                                   uint16_t error_code,
                                   const URLEntries &urls) {
  // erase xid from the map
  m_pending_replies.erase(op->xid);

  OLA_INFO << "Got DA SrvReply, error code is " << error_code;
  if (error_code == SLP_OK) {
    std::copy(urls.begin(), urls.end(), std::back_inserter(op->parent->urls));
    // mark all these scopes as complete
    for (ScopeSet::Iterator iter = op->scopes.begin(); iter != op->scopes.end();
         ++iter) {
      op->parent->MarkScopeAsDone(*iter);
    }
    CheckIfFindSrvComplete(op->parent);
    m_ss->RemoveTimeout(op->timer_id);
    delete op;
  } else if (error_code == DA_BUSY_NOW) {
    // mark the DA as busy and let the timeout expire so we retry.
    op->da_busy = true;
  } else {
    // declare this DA bad
    OLA_INFO << "Declaring DA " << op->da_url << " bad due to error code";
    m_ss->RemoveTimeout(op->timer_id);
    m_da_tracker.MarkAsBad(op->da_url);
    FindServiceInScopes(op->parent, op->scopes);
    delete op;
  }
}


/**
 * Called when a SrvRqst to a DA times out. This may trigger a retry or, if
 * we've hit the retry limit we'll move on to another DA, or fall back to
 * multicast.
 * This assumes ownership of op.
 */
void SLPServer::RequestServiceDATimeout(UnicastSrvRqstOperation *op) {
  OLA_INFO << "SrvRqst to " << op->da_url << " timed out";
  auto_ptr<UnicastSrvRqstOperation> op_deleter(op);

  PendingReplyMap::iterator iter = m_pending_replies.find(op->xid);
  if (iter == m_pending_replies.end()) {
    OLA_WARN << "Unable to find matching xid: " << op->xid;
    return;
  }

  op->UpdateRetryTime();
  if (op->total_time() + op->retry_time() > m_config_retry_max) {
    // this DA is bad
    OLA_INFO << "Declaring DA " << op->da_url << " bad since total time is now "
             << op->total_time();
    m_da_tracker.MarkAsBad(op->da_url);
    CancelPendingSrvRqstAck(iter);
    FindServiceInScopes(op->parent, op->scopes);
    return;
  }

  DirectoryAgent da;
  bool failed = false;
  if (!m_da_tracker.LookupDA(op->da_url, &da)) {
    // This DA no longer exists
    OLA_WARN << "DA " << op->da_url << " no longer exists";
    failed = true;
  } else if (!da.scopes().Intersects(op->scopes)) {
    OLA_WARN << "DA " << op->da_url << " no longer has scopes that match "
             << op->scopes;
    failed = true;
  }

  if (failed) {
    CancelPendingSrvRqstAck(iter);
    FindServiceInScopes(op->parent, op->scopes);
    return;
  }

  // we're going to reuse the op, so don't delete it.
  op_deleter.release();
  // we expect a XID collison here.
  SendSrvRqstToDA(op, da, true);
}


/**
 * Remove a pending SrvAck from the map, and delete the callback
 */
void SLPServer::CancelPendingSrvRqstAck(const PendingReplyMap::iterator &iter) {
  delete iter->second;
  m_pending_replies.erase(iter);
}


/**
 * Called when a multicast SrvRqst request times out.
 * It's not really clear from Section 6.3 what the terminating condition for
 * this is. My interpretation is that we terminate if:
 *  - no new responses were received
 *  - the message no longer fits in a datagram
 *  - CONFIG_MC_MAX is reached
 */
void SLPServer::RequestServiceMulticastTimeout(
    MulicastSrvRqstOperation *op) {
  OLA_INFO << "xid " << op->xid << " timeout, attempt "
           << static_cast<unsigned int>(op->AttemptNumber());
  bool first_attempt = op->AttemptNumber() == 1;
  op->UpdateRetryTime();
  PendingReplyMap::iterator iter = m_pending_replies.find(op->xid);
  if (iter == m_pending_replies.end()) {
    OLA_WARN << "Can't find callback for xid " << op->xid << ", this is a bug!";
    return;
  }

  // make sure we always send the SrvRqst at least twice. The RFC isn't
  // too clear about this, (6.3), but this protects against a dropped packet.
  if ((!op->PRListChanged() && !first_attempt) ||
      op->PRListSize() > MAX_PR_LIST_SIZE ||
      op->total_time() >= m_config_mc_max) {
    // We're done, cleaning up the multi-use callback
    delete iter->second;
    m_pending_replies.erase(iter);
    for (ScopeSet::Iterator iter = op->scopes.begin(); iter != op->scopes.end();
         ++iter)
      op->parent->MarkScopeAsDone(*iter);
    CheckIfFindSrvComplete(op->parent);
    delete op;
    return;
  }

  if (op->PRListChanged()) {
    op->ResetPRListChanged();
    // we need a new xid now, reuse the callback though
    SrvReplyCallback *cb = iter->second;
    m_pending_replies.erase(iter);
    op->xid = m_xid_allocator.Next();
    pair<xid_t, SrvReplyCallback*> p(op->xid, cb);
    if (!m_pending_replies.insert(p).second)
      OLA_WARN << "Collision for xid " << op->xid
               << ", we're probably leaking memory!";
  }

  OLA_INFO << "Retry time for " << op->xid << " is now " << op->retry_time();
  op->timer_id = m_ss->RegisterSingleTimeout(
      op->retry_time(),
      NewSingleCallback(this, &SLPServer::RequestServiceMulticastTimeout, op));
  m_udp_sender.SendServiceRequest(m_multicast_endpoint, op->xid, op->pr_list,
                                  op->parent->service_type, op->scopes);
}


/**
 * Called when we receive a response to a multicast SrvRqst
 */
void SLPServer::ReceivedSASrvReply(MulicastSrvRqstOperation *op,
                                   const IPV4Address &src,
                                   uint16_t error_code,
                                   const URLEntries &urls) {
  OLA_INFO << "Got SrvReply with code " << error_code;
  if (error_code == SLP_OK) {
    // add the URLEntries add put this in the PR list
    std::copy(urls.begin(), urls.end(), std::back_inserter(op->parent->urls));
    op->AddPR(src);
  } else {
    // we should never get an error here
    OLA_WARN << "Got non-0 error code (" << error_code << ") from "<< src;
  }
}


/**
 * Check if the Find Service request is complete. This is true if all scopes
 * have completed. If the request is complete, we execute the callback and
 * delete the PendingSrvRqst object.
 */
void SLPServer::CheckIfFindSrvComplete(PendingSrvRqst *request) {
  if (!request->Complete())
    return;

  // ok we're done
  request->callback->Run(request->urls);
  delete request;
}


// SA methods
//------------------------------------------------------------------------------

/**
 * Cancel any pending DA Reg / DeReg operations for this URL
 * @param url the url to cancel operations for.
 */
void SLPServer::CancelPendingDAOperationsForService(const string &url) {
  PendingOperationsByURL::iterator iter;
  PendingOperationsByURL::iterator lower = m_pending_ops.lower_bound(url);
  PendingOperationsByURL::iterator upper = m_pending_ops.upper_bound(url);

  for (iter = lower; iter != upper; ++iter) {
    FreePendingDAOperation(iter->second);
  }
  m_pending_ops.erase(lower, upper);
}


/**
 * Cancel any pending DA Reg / DeReg operations for this (URL , DA URL) pair.
 * @param url the url to cancel operations for.
 * @param da_url the DA url to cancel operations for.
 */
void SLPServer::CancelPendingDAOperationsForServiceAndDA(const string &url,
                                                         const string &da_url) {
  PendingOperationsByURL::iterator iter = m_pending_ops.lower_bound(url);
  PendingOperationsByURL::iterator upper = m_pending_ops.upper_bound(url);
  // take a copy of the da_url, since it may be a reference into an object
  // we're about to delete
  const string our_da_url = da_url;

  while (iter != upper) {
    if (iter->second->da_url == our_da_url) {
      FreePendingDAOperation(iter->second);
      m_pending_ops.erase(iter++);
    } else {
      iter++;
    }
  }
}


/**
 * Free the resources associated with a pending Reg/DeReg operation.
 */
void SLPServer::FreePendingDAOperation(UnicastSrvRegOperation *op) {
  m_ss->RemoveTimeout(op->timer_id);  // cancel the timer

  PendingAckMap::iterator ack_iter = m_pending_acks.find(op->xid);
  if (ack_iter != m_pending_acks.end()) {
    delete ack_iter->second;
    m_pending_acks.erase(ack_iter);
  }
  delete op;
}


/**
 * Register a service. May register with DAs if we know about any.
 */
uint16_t SLPServer::InternalRegisterService(const ServiceEntry &service) {
  TimeStamp now;
  GetCurrentTime(&now);

  SLPStore::ReturnCode result = m_service_store.CheckIfScopesMatch(now,
                                                                   service);
  if (result == SLPStore::SCOPE_MISMATCH)
    return SCOPE_NOT_SUPPORTED;

  CancelPendingDAOperationsForService(service.url_string());

  // TODO(simon): use the error from here, and maybe skip the DA part
  m_service_store.Insert(now, service);

  vector<DirectoryAgent> directory_agents;
  m_da_tracker.GetDAsForScopes(service.scopes(), &directory_agents);
  for (vector<DirectoryAgent>::iterator da_iter = directory_agents.begin();
       da_iter != directory_agents.end(); ++da_iter) {
    RegisterWithDA(*da_iter, service);
  }
  return SLP_OK;
}


/**
 * Register a service. May register with DAs if we know about any.
 */
uint16_t SLPServer::InternalDeRegisterService(const ServiceEntry &service) {
  TimeStamp now;
  GetCurrentTime(&now);

  SLPStore::ReturnCode result = m_service_store.CheckIfScopesMatch(now,
                                                                   service);
  if (result == SLPStore::SCOPE_MISMATCH)
    return SCOPE_NOT_SUPPORTED;
  else if (result == SLPStore::NOT_FOUND)
    return SLP_OK;

  CancelPendingDAOperationsForService(service.url_string());

  vector<DirectoryAgent> directory_agents;
  // This only works correctly if we assume DAs can't change scopes
  // if a DA changes scopes it's not really clear what we're supposed to do
  m_da_tracker.GetDAsForScopes(service.scopes(), &directory_agents);
  for (vector<DirectoryAgent>::iterator da_iter = directory_agents.begin();
       da_iter != directory_agents.end(); ++da_iter) {
    DeRegisterWithDA(*da_iter, service);
  }

  // TODO(simon): use the error from here, and maybe skip the DA part
  m_service_store.Remove(service);
  return SLP_OK;
}


/**
 * SrvAck callback for SrvReg and SrvDeReg requests.
 */
void SLPServer::ReceivedAck(UnicastSrvRegOperation *op,
                            uint16_t error_code) {
  if (error_code == DA_BUSY_NOW)
    // This is the same as a failure, so let the timeout expire.
    // TODO(simon): does this count towards marking a DA as bad?
    return;

  if (error_code)
    OLA_WARN << "xid " << op->xid << " returned " << error_code << " : "
             << SLPErrorToString(error_code);
  else
    OLA_INFO << "xid " << op->xid << " was acked";

  // this deletes the timeout, and the UnicastSrvRegOperation
  CancelPendingDAOperationsForServiceAndDA(op->service.url().url(),
                                           op->da_url);
}


/*
 * The timeout handler for SrvReg requests.
 */
void SLPServer::RegistrationTimeout(UnicastSrvRegOperation *op) {
  UnicastOperationDeleter op_deleter(op, this);

  PendingAckMap::iterator iter = m_pending_acks.find(op->xid);
  if (iter == m_pending_acks.end()) {
    OLA_WARN << "Unable to find matching xid: " << op->xid;
    return;
  }

  OLA_INFO << "in timeout, retry was " << op->retry_time();
  if (op->service.mutable_url().AgeLifetime(op->retry_time() / 1000)) {
    // this service has expired while we're trying to register it
    OLA_INFO << "Service " << op->service << " expired during registration.";
    CancelPendingSrvAck(iter);
    return;
  }

  op->UpdateRetryTime();
  if (op->total_time() + op->retry_time() > m_config_retry_max) {
    // this DA is bad
    OLA_INFO << "Declaring DA " << op->da_url << " bad since total time is now "
             << op->total_time();
    m_da_tracker.MarkAsBad(op->da_url);
    CancelPendingSrvAck(iter);
    return;
  }

  DirectoryAgent da;
  if (!m_da_tracker.LookupDA(op->da_url, &da)) {
    // This DA no longer exists
    OLA_WARN << "DA " << op->da_url << " no longer exists";
    CancelPendingSrvAck(iter);
    return;
  }

  ScopeSet scopes_to_use = da.scopes().Intersection(op->service.scopes());
  if (scopes_to_use.empty()) {
    OLA_INFO << "DA " << op->da_url << " no longer has scopes that match "
             << op->service;
    CancelPendingSrvAck(iter);
    return;
  }

  // we're going to reuse the op, so don't delete it.
  op_deleter.Cancel();

  // if the scopes are different, do we need a new xid?
    // TODO(simon)
    // xid_t xid = m_xid_allocator.Next();

  m_udp_sender.SendServiceRegistration(
      IPV4SocketAddress(da.IPAddress(), m_slp_port),
      op->xid, true, scopes_to_use, op->service);

  op->timer_id = m_ss->RegisterSingleTimeout(
      op->retry_time(),
      NewSingleCallback(this, &SLPServer::RegistrationTimeout, op));
}


/**
 * The timeout handler for SrvDeReg requests
 */
void SLPServer::DeRegistrationTimeout(UnicastSrvRegOperation *op) {
  UnicastOperationDeleter op_deleter(op, this);

  PendingAckMap::iterator iter = m_pending_acks.find(op->xid);
  if (iter == m_pending_acks.end()) {
    OLA_WARN << "Unable to find matching xid: " << op->xid;
    return;
  }

  // ok, we need to re-try
  op->UpdateRetryTime();
  if (op->total_time() + op->retry_time() >= m_config_retry_max) {
    // this DA is bad
    OLA_INFO << "Declaring DA " << op->da_url << " bad since total time is now "
             << op->total_time();
    m_da_tracker.MarkAsBad(op->da_url);
    CancelPendingSrvAck(iter);
    return;
  }

  DirectoryAgent da;
  if (!m_da_tracker.LookupDA(op->da_url, &da)) {
    // This DA no longer exists
    OLA_WARN << "DA " << op->da_url << " no longer exists";
    CancelPendingSrvAck(iter);
    return;
  }

  ScopeSet scopes_to_use = da.scopes().Intersection(op->service.scopes());

  // we're going to reuse the op, so don't delete it.
  op_deleter.Cancel();

  // It's not clear which scopes we should use here if the DA has changed since
  // we registered. For now we attempt to DeReg with the exact same scopes that
  // we registered with (Section 8.3)
  m_udp_sender.SendServiceDeRegistration(
      IPV4SocketAddress(da.IPAddress(), m_slp_port), op->xid,
      scopes_to_use, op->service);

  op->timer_id = m_ss->RegisterSingleTimeout(
      op->retry_time(),
      NewSingleCallback(this, &SLPServer::DeRegistrationTimeout, op));
}


/*
 * Register a service with a DA. We only register for the scopes each DA
 * supports.
 * @param directory_agents the DA to register with
 * @param service the service to register
 */
void SLPServer::RegisterWithDA(const DirectoryAgent &agent,
                               const ServiceEntry &service) {
  OLA_INFO << "Registering " << service << " with " << agent;
  UnicastSrvRegOperation *op = new UnicastSrvRegOperation(
      m_xid_allocator.Next(), m_config_retry, agent.URL(), service);
  op->timer_id = m_ss->RegisterSingleTimeout(
      op->retry_time(),
      NewSingleCallback(this, &SLPServer::RegistrationTimeout, op));
  m_pending_ops.insert(
      pair<string, UnicastSrvRegOperation*>(service.url().url(), op));

  ScopeSet scopes_to_use = agent.scopes().Intersection(service.scopes());
  m_udp_sender.SendServiceRegistration(
      IPV4SocketAddress(agent.IPAddress(), m_slp_port),
      op->xid, true, scopes_to_use, service);

  AddPendingSrvAck(op->xid,
                   NewSingleCallback(this, &SLPServer::ReceivedAck, op));
}


/*
 * De-Register a service with a DA. We only register for the scopes each DA
 * supports.
 * @param directory_agents the DA to deregister with
 * @param service the service to deregister
 */
void SLPServer::DeRegisterWithDA(const DirectoryAgent &agent,
                                 const ServiceEntry &service) {
  OLA_INFO << "DeRegistering " << service << " with " << agent;
  UnicastSrvRegOperation *op = new UnicastSrvRegOperation(
      m_xid_allocator.Next(), m_config_retry, agent.URL(), service);
  op->timer_id = m_ss->RegisterSingleTimeout(
      op->retry_time(),
      NewSingleCallback(this, &SLPServer::DeRegistrationTimeout, op));
  m_pending_ops.insert(
      pair<string, UnicastSrvRegOperation*>(service.url().url(), op));

  // send message to DA
  // TODO(simon): how do we know what scopes to de-register with?
  ScopeSet scopes_to_use = agent.scopes().Intersection(service.scopes());
  m_udp_sender.SendServiceDeRegistration(
      IPV4SocketAddress(agent.IPAddress(), m_slp_port), op->xid, scopes_to_use,
      service);

  AddPendingSrvAck(op->xid,
                   NewSingleCallback(this, &SLPServer::ReceivedAck, op));
}


/**
 * Remove a pending SrvAck from the map, and delete the callback
 */
void SLPServer::CancelPendingSrvAck(const PendingAckMap::iterator &iter) {
  delete iter->second;
  m_pending_acks.erase(iter);
}


/**
 * Associate a callback with a xid.
 */
void SLPServer::AddPendingSrvAck(xid_t xid, AckCallback *callback) {
  OLA_INFO << "adding callback for " << xid;
  pair<xid_t, AckCallback*> p(xid, callback);
  if (!m_pending_acks.insert(p).second)
    OLA_WARN << "Collision for xid " << xid
             << ", we're probably leaking memory!";
}


/**
 * Send a Service Request for 'directory-agent'
 */
void SLPServer::StartActiveDADiscovery() {
  if (m_outstanding_da_discovery.get()) {
    OLA_INFO << "Active DA Discovery already running.";
    return;
  }
  m_outstanding_da_discovery.reset(
      new PendingMulticastOperation(m_xid_allocator.Next(), m_config_retry));
  SendDARequestAndSetupTimer(m_outstanding_da_discovery.get());
}


/**
 * Called when we timeout a SrvRqst for service:directory-agent.
 */
void SLPServer::DASrvRqstTimeout() {
  if (!m_outstanding_da_discovery.get()) {
    OLA_WARN << "DA Tick but no outstanding DA request";
    ScheduleActiveDADiscovery();
    return;
  }

  PendingMulticastOperation *op = m_outstanding_da_discovery.get();
  bool first_attempt = op->AttemptNumber() == 1;

  op->UpdateRetryTime();
  // make sure we always send the SrvRqst at least twice. The RFC isn't
  // too clear about this, (6.3), but this protects against a dropped packet.
  if ((!op->PRListChanged() && !first_attempt) ||
      op->PRListSize() > MAX_PR_LIST_SIZE ||
      op->total_time() >= m_config_mc_max) {
    // we've come to the end of the road jack
    m_outstanding_da_discovery.reset();
    ScheduleActiveDADiscovery();
    OLA_INFO << "Active DA discovery complete";
  } else {
    SendDARequestAndSetupTimer(op);
  }
}


/**
 * Send a SrvRqst for service:directory-agent and scheduler a timeout.
 */
void SLPServer::SendDARequestAndSetupTimer(PendingMulticastOperation *op) {
  if (op->PRListChanged()) {
    op->ResetPRListChanged();
    // because the PR list changed we should use a new xid
    op->xid = m_xid_allocator.Next();
  }
  m_udp_sender.SendServiceRequest(m_multicast_endpoint, op->xid,
                                  op->pr_list, DIRECTORY_AGENT_SERVICE,
                                  m_configured_scopes);
  op->timer_id = m_ss->RegisterSingleTimeout(
      op->retry_time(),
      NewSingleCallback(this, &SLPServer::DASrvRqstTimeout));
}


/**
 * Schedule the next active DA discovery run.
 */
void SLPServer::ScheduleActiveDADiscovery() {
  m_active_da_discovery_timer = m_ss->RegisterSingleTimeout(
      m_config_da_find,
      NewSingleCallback(this, &SLPServer::StartActiveDADiscovery));
}


/**
 * Called when the DA Tracker locates a new DA on the network.
 */
void SLPServer::NewDACallback(const DirectoryAgent &agent) {
  m_ss->RegisterSingleTimeout(
      Random(m_config_reg_active_min, m_config_reg_active_max),
      NewSingleCallback(this, &SLPServer::RegisterServicesWithNewDA,
                        agent.URL()));
}


/**
 * Register all of the relevent services with a DA
 */
void SLPServer::RegisterServicesWithNewDA(const string da_url) {
  DirectoryAgent da;
  if (!m_da_tracker.LookupDA(da_url, &da)) {
    OLA_INFO << "DA " << da_url << " no longer exists";
    return;
  }
  OLA_INFO << "Registering local services with " << da_url;

  ServiceEntries services;
  m_service_store.GetLocalServices(*(m_ss->WakeUpTime()), da.scopes(),
                                   &services);

  // Go through out local services and see if any need to be registered with
  // this DA.
  for (ServiceEntries::const_iterator iter = services.begin();
       iter != services.end(); ++iter) {
    RegisterWithDA(da, *iter);
  }
}


/**
 * Tidy up the SLP store
 */
bool SLPServer::CleanSLPStore() {
  m_service_store.Clean(*(m_ss->WakeUpTime()));
  return true;
}


/**
 * Increment the method counter for the specified method.
 */
void SLPServer::IncrementMethodVar(const string &method) {
  if (m_export_map)
    m_export_map->GetUIntMapVar(METHOD_CALLS_VAR)->Increment(method);
}


/**
 * Increment the packet counter for the specified packet type.
 */
void SLPServer::IncrementPacketVar(const string &packet) {
  if (m_export_map)
    m_export_map->GetUIntMapVar(UDP_RX_PACKET_BY_TYPE_VAR)->Increment(packet);
}


/**
 * Get the current time, either from the Clock object given to us or the
 * default clock.
 */
void SLPServer::GetCurrentTime(TimeStamp *time) {
  if (m_clock) {
    m_clock->CurrentTime(time);
  } else {
    ola::Clock clock;
    clock.CurrentTime(time);
  }
}


/**
 * Check if we're in a PR list.
 */
bool SLPServer::InPRList(const vector<IPV4Address> &pr_list) {
  return std::find(pr_list.begin(), pr_list.end(), m_iface_address) !=
    pr_list.end();
}
}  // slp
}  // ola
