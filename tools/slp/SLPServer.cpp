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

#include "tools/slp/ServerCommon.h"
#include "tools/slp/SLPPacketBuilder.h"
#include "tools/slp/SLPPacketParser.h"
#include "tools/slp/SLPServer.h"
#include "tools/slp/SLPStore.h"
#include "tools/slp/SLPStrings.h"
#include "tools/slp/SLPUtil.h"

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
using ola::network::UDPSocket;
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
      m_slp_port(options.slp_port),
      m_config_da_beat(options.config_da_beat * ONE_THOUSAND),
      m_config_da_find(options.config_da_find * ONE_THOUSAND),
      m_config_mc_max(options.config_mc_max * ONE_THOUSAND),
      m_config_retry(options.config_retry * ONE_THOUSAND),
      m_config_retry_max(options.config_retry_max * ONE_THOUSAND),
      m_config_start_wait(options.config_start_wait * ONE_THOUSAND),
      m_en_lang(reinterpret_cast<const char*>(EN_LANGUAGE_TAG),
                sizeof(EN_LANGUAGE_TAG)),
      m_iface_address(options.ip_address),
      m_multicast_endpoint(IPV4SocketAddress(
            IPV4Address(HostToNetwork(SLP_MULTICAST_ADDRESS)), m_slp_port)),
      m_ss(ss),
      m_da_beat_timer(ola::thread::INVALID_TIMEOUT),
      m_store_cleaner_timer(ola::thread::INVALID_TIMEOUT),
      m_active_da_discovery_timer(ola::thread::INVALID_TIMEOUT),
      m_udp_socket(udp_socket),
      m_slp_accept_socket(tcp_socket),
      m_udp_sender(m_udp_socket),
      m_configured_scopes(options.scopes),
      m_xid_allocator(Random(0, UINT16_MAX)),
      m_export_map(export_map) {
  ToLower(&m_en_lang);

  if (m_configured_scopes.empty())
    m_configured_scopes = ScopeSet(DEFAULT_SLP_SCOPE);

  export_map->GetBoolVar("slp-da-enabled")->Set(options.enable_da);
  export_map->GetIntegerVar("slp-config-da-beat")->Set(options.config_da_beat);
  export_map->GetIntegerVar("slp-config-da-find")->Set(options.config_da_find);
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


SLPServer::~SLPServer() {
  m_ss->RemoveTimeout(m_da_beat_timer);
  m_ss->RemoveTimeout(m_store_cleaner_timer);
  m_ss->RemoveTimeout(m_active_da_discovery_timer);

  for (PendingOperationsByURL::iterator iter = m_pending_ops.begin();
       iter != m_pending_ops.end(); ++iter) {
    m_ss->RemoveTimeout(iter->second->timer_id);
    delete iter->second;
  }

  m_udp_socket->Close();
  STLDeleteValues(m_pending_acks);
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
    ola::Clock clock;
    clock.CurrentTime(&m_boot_time);

    // setup the DA beat timer
    m_da_beat_timer = m_ss->RegisterRepeatingTimeout(
        m_config_da_beat * 1000,
        NewCallback(this, &SLPServer::SendDABeat));
    SendDABeat();
  } else {
    // schedule a SrvRqst for the directory agent
    m_active_da_discovery_timer = m_ss->RegisterSingleTimeout(
        Random(0, m_config_start_wait),
        NewSingleCallback(this, &SLPServer::StartActiveDADiscovery));
  }
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
    SingleUseCallback1<void, const URLEntries&> *cb) {
  m_export_map->GetUIntMapVar(METHOD_CALLS_VAR)->Increment(METHOD_FIND_SERVICE);
  ScopeSet scope_set(scopes);
  URLEntries urls;

  if (!m_enable_da) {
    InternalFindService(scopes, service_type, cb);
    return;
  }

  // we're in DA mode, just return all the matching ServiceEntries we know
  // about.
  if (scope_set.Intersects(m_configured_scopes)) {
    OLA_INFO << "Received SrvRqst for " << service_type;
    LocalLocateServices(scope_set, service_type, &urls);
  }

  if (urls.empty())
    (*m_export_map->GetIntegerVar(FINDSRVS_EMPTY_COUNT_VAR))++;
  cb->Run(urls);
}


/**
 * Register a service
 * @param service the ServiceEntry to register
 * @returns an SLP error code
 */
uint16_t SLPServer::RegisterService(const ServiceEntry &new_service) {
  m_export_map->GetUIntMapVar(METHOD_CALLS_VAR)->Increment(METHOD_REG_SERVICE);
  ServiceEntry service(new_service);
  service.set_local(true);

  uint16_t error_code;
  if (!service.url().lifetime()) {
    OLA_WARN << "Attempt to register " << service << " with a lifetime of 0";
    error_code = INVALID_REGISTRATION;
  } else {
    error_code = InternalRegisterService(service);
  }
  if (error_code)
    (*m_export_map->GetIntegerVar(REGSRVS_ERROR_COUNT_VAR))++;
  return error_code;
}


/**
 * DeRegister a service
 * @param service the ServiceEntry to de-register
 * @returns an SLP error code
 */
uint16_t SLPServer::DeRegisterService(const ServiceEntry &service) {
  m_export_map->GetUIntMapVar(METHOD_CALLS_VAR)->Increment(
      METHOD_DEREG_SERVICE);

  uint16_t error_code = InternalDeRegisterService(service);
  if (error_code)
    (*m_export_map->GetIntegerVar(DEREGSRVS_ERROR_COUNT_VAR))++;
  return error_code;
}


/**
 * Called when there is data on the UDP socket
 */
void SLPServer::UDPData() {
  // TODO(simon): make this a member when we're done
  ssize_t packet_size = 1500;
  uint8_t packet[packet_size];
  ola::network::IPV4Address source_ip;
  uint16_t port;

  if (!m_udp_socket->RecvFrom(reinterpret_cast<uint8_t*>(&packet),
                              &packet_size, source_ip, port))
    return;
  IPV4SocketAddress source(source_ip, port);

  OLA_DEBUG << "Got " << packet_size << " UDP bytes from " << source;
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
  if (request->scope_list.empty()) {
    SendErrorIfUnicast(request.get(), source, SCOPE_NOT_SUPPORTED);
    return;
  }
  ScopeSet scope_set(request->scope_list);
  if (!scope_set.Intersects(m_configured_scopes)) {
    SendErrorIfUnicast(request.get(), source, SCOPE_NOT_SUPPORTED);
    return;
  }

  URLEntries urls;
  OLA_INFO << "Received SrvRqst for " << request->service_type;
  LocalLocateServices(scope_set, request->service_type, &urls);

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
  const ServiceReplyPacket *srv_reply =
    m_packet_parser.UnpackServiceReply(stream);
  if (!srv_reply)
    return;

  OLA_INFO << "Unpacked service reply, error_code: " << srv_reply->error_code;
  delete srv_reply;
}


/**
 * Handle a Service Registration packet, only DAs support this.
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
  auto_ptr<const ServiceAckPacket> srv_ack(
      m_packet_parser.UnpackServiceAck(stream));
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
  // Running the callback may change the map, invalidating the iterator
  AckCallback *cb = iter->second;
  m_pending_acks.erase(iter);
  cb->Run(srv_ack->error_code);
}


/**
 * Handle a DAAdvert.
 */
void SLPServer::HandleDAAdvert(BigEndianInputStream *stream,
                               const IPV4SocketAddress &source) {
  OLA_INFO << "Got DAAdvert from " << source;
  auto_ptr<const DAAdvertPacket> da_advert(
      m_packet_parser.UnpackDAAdvert(stream));
  if (!da_advert.get())
    return;

  if (da_advert->error_code) {
    OLA_WARN << "DAAdvert from " << source << ", had error "
             << da_advert->error_code << " ("
             << SLPErrorToString(da_advert->error_code) << ")";
    return;
  }

  if (m_outstanding_da_discovery.get()) {
    // active discovery in progress
    m_outstanding_da_discovery->AddPR(source.Host());
  }
  m_da_tracker.NewDAAdvert(*da_advert, source);
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
  ScopeSet scopes(request->scope_list);
  if (!(scopes.empty() || scopes.Intersects(m_configured_scopes))) {
    SendErrorIfUnicast(request, source, SCOPE_NOT_SUPPORTED);
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
  if (!(scopes.empty() || scopes.Intersects(m_configured_scopes))) {
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
                            str.str(), m_configured_scopes);
}

/**
 * Send a multicast DAAdvert packet
 */
bool SLPServer::SendDABeat() {
  SendDAAdvert(m_multicast_endpoint);
  return true;
}


/**
 * Lookup a service in our local stores
 * @param scopes the list of canonical scopes
 * @param service_type the service to locate.
 * @param services a ServiceEntries where we store the results
 */
void SLPServer::LocalLocateServices(const ScopeSet &scopes,
                                    const string &service_type,
                                    URLEntries *services) {
  m_service_store.Lookup(*(m_ss->WakeUpTime()), scopes, service_type, services);
}


/**
 * Locate a service, Uses DAs if they exist, or otherwise falls back to
 * multicasting.
 */
void SLPServer::InternalFindService(
    const set<string> &scopes,
    const string &service,
    SingleUseCallback1<void, const URLEntries&> *cb) {

  (void) scopes;
  (void) service;
  (void) cb;
}


/**
 * Cancel any pending operations for this URL
 */
void SLPServer::CancelPendingOperations(const string &url) {
  PendingOperationsByURL::iterator iter;
  PendingOperationsByURL::iterator lower = m_pending_ops.lower_bound(url);
  PendingOperationsByURL::iterator upper = m_pending_ops.upper_bound(url);

  for (iter = lower; iter != upper; ++iter) {
    m_ss->RemoveTimeout(iter->second->timer_id);  // cancel the timer
    m_pending_acks.erase(iter->second->xid);
    delete iter->second;
  }
  m_pending_ops.erase(lower, upper);
}


/**
 * Register a service. May register with DAs if we know about any.
 */
uint16_t SLPServer::InternalRegisterService(const ServiceEntry &service) {
  TimeStamp now;
  ola::Clock clock;
  clock.CurrentTime(&now);

  SLPStore::ReturnCode result = m_service_store.CheckIfScopesMatch(now,
                                                                   service);
  if (result == SLPStore::SCOPE_MISMATCH)
    return SCOPE_NOT_SUPPORTED;

  CancelPendingOperations(service.url_string());

  m_service_store.Insert(now, service);

  vector<DirectoryAgent> directory_agents;
  m_da_tracker.GetDAsForScopes(service.scopes(), &directory_agents);
  for (vector<DirectoryAgent>::iterator da_iter = directory_agents.begin();
       da_iter != directory_agents.end(); ++da_iter) {
    OLA_INFO << "registering with " << *da_iter;
    RegisterWithDA(*da_iter, service);
  }
  return SLP_OK;
}


/**
 * Register a service. May register with DAs if we know about any.
 */
uint16_t SLPServer::InternalDeRegisterService(const ServiceEntry &service) {
  TimeStamp now;
  ola::Clock clock;
  clock.CurrentTime(&now);

  SLPStore::ReturnCode result = m_service_store.CheckIfScopesMatch(now,
                                                                   service);
  if (result == SLPStore::SCOPE_MISMATCH)
    return SCOPE_NOT_SUPPORTED;
  else if (result == SLPStore::NOT_FOUND)
    return SLP_OK;

  CancelPendingOperations(service.url_string());

  vector<DirectoryAgent> directory_agents;
  // This only works correctly if we assume DAs can't change scopes
  // if a DA changes scopes it's not really clear what we're supposed to do
  m_da_tracker.GetDAsForScopes(service.scopes(), &directory_agents);
  for (vector<DirectoryAgent>::iterator da_iter = directory_agents.begin();
       da_iter != directory_agents.end(); ++da_iter) {
    DeRegisterWithDA(*da_iter, service);
  }
  m_service_store.Remove(service);
  return SLP_OK;
}


/**
 * SrvAck callback for SrvReg and SrvDeReg requests.
 */
void SLPServer::ReceivedAck(PendingOperation *op_ptr, uint16_t error_code) {
  if (error_code == DA_BUSY_NOW)
    // This is the same as a failure, so let the timeout expire.
    // TODO(simon): does this count towards marking a DA as bad?
    return;

  auto_ptr<PendingOperation> op(op_ptr);
  if (error_code)
    OLA_WARN << "xid " << op->xid << " returned " << error_code << " : "
             << SLPErrorToString(error_code);
  else
    OLA_INFO << "xid " << op->xid << " was acked";

  m_ss->RemoveTimeout(op->timer_id);
}


/*
 * The timeout handler for SrvReg requests.
 */
void SLPServer::RegistrationTimeout(PendingOperation *op) {
  auto_ptr<PendingOperation> op_deleter(op);

  PendingAckMap::iterator iter = m_pending_acks.find(op->xid);
  if (iter == m_pending_acks.end()) {
    OLA_WARN << "Unable to find matching xid: " << op->xid;
    return;
  }

  OLA_INFO << "in timeout, retry was " << op->retry_time;
  if (op->retry_time > m_config_retry_max) {
    // this DA is bad
    OLA_INFO << "Declaring DA " << op->da_url << " bad since retry is now "
             << op->retry_time;
    m_da_tracker.MarkAsBad(op->da_url);
    CancelPendingSrvAck(iter);
    return;
  }

  op->retry_time += op->retry_time;
  OLA_INFO << "in timeout, retry is now " << op->retry_time;

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
  op_deleter.release();

  // Do we want to age the service lifetime here?
  // op->service.mutable_url.set_lifetime(

  // if the scopes are different, do we need a new xid?
    // TODO(simon)
    // xid_t xid = m_xid_allocator.Next();

  m_udp_sender.SendServiceRegistration(
      IPV4SocketAddress(da.IPAddress(), m_slp_port),
      op->xid, true, scopes_to_use, op->service);

  op->timer_id = m_ss->RegisterSingleTimeout(
      op->retry_time,
      NewSingleCallback(this, &SLPServer::RegistrationTimeout, op));
}


/**
 * The timeout handler for SrvDeReg requests
 */
void SLPServer::DeRegistrationTimeout(PendingOperation *op) {
  auto_ptr<PendingOperation> op_deleter(op);

  PendingAckMap::iterator iter = m_pending_acks.find(op->xid);
  if (iter == m_pending_acks.end()) {
    OLA_WARN << "Unable to find matching xid: " << op->xid;
    return;
  }

  // ok, we need to re-try
  op->retry_time += op->retry_time;
  if (op->retry_time > m_config_retry_max) {
    // this DA is bad
    OLA_INFO << "Declaring DA " << op->da_url << " bad since retry is now "
             << op->retry_time;
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
  op_deleter.release();

  // It's not clear which scopes we should use here if the DA has changed since
  // we registered. For now we attempt to DeReg with the exact same scopes that
  // we registered with (Section 8.3)
  m_udp_sender.SendServiceDeRegistration(
      IPV4SocketAddress(da.IPAddress(), m_slp_port), op->xid,
      scopes_to_use, op->service);

  op->timer_id = m_ss->RegisterSingleTimeout(
      op->retry_time,
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
  PendingOperation *op = new PendingOperation(service, m_xid_allocator.Next(),
                                              m_config_retry);
  op->da_url = agent.URL();
  op->service = service;
  op->timer_id = m_ss->RegisterSingleTimeout(
      op->retry_time,
      NewSingleCallback(this, &SLPServer::RegistrationTimeout, op));

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
  PendingOperation *op = new PendingOperation(service, m_xid_allocator.Next(),
                                              m_config_retry);
  op->da_url = agent.URL();
  op->service = service;
  op->timer_id = m_ss->RegisterSingleTimeout(
      op->retry_time,
      NewSingleCallback(this, &SLPServer::DeRegistrationTimeout, op));

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
      new OutstandingDADiscovery(m_xid_allocator.Next()));
  SendDARequestAndSetupTimer(m_outstanding_da_discovery.get());
}


/**
 * Called when we timeout a SrvRqst for service:directory-agent.
 */
void SLPServer::ActiveDATick() {
  if (!m_outstanding_da_discovery.get()) {
    OLA_WARN << "DA Tick but no outstanding DA request";
    ScheduleActiveDADiscovery();
    return;
  }

  // TODO(simon): also terminate when the PR list is too big
  if (m_outstanding_da_discovery->attempts_remaining == 0) {
    // we've come to the end of the road jack
    m_outstanding_da_discovery.reset();
    ScheduleActiveDADiscovery();
    OLA_INFO << "Active DA discovery complete";
    return;
  } else {
    SendDARequestAndSetupTimer(m_outstanding_da_discovery.get());
  }
}


/**
 * Send a SrvRqst for service:directory-agent and scheduler a timeout.
 */
void SLPServer::SendDARequestAndSetupTimer(OutstandingDADiscovery *request) {
  if (request->PRListChanged()) {
    request->ResetPRListChanged();
    // because the PR list changed we should use a new xid
    request->xid = m_xid_allocator.Next();
  }
  m_udp_sender.SendServiceRequest(m_multicast_endpoint, request->xid,
                                  request->pr_list, DIRECTORY_AGENT_SERVICE,
                                  m_configured_scopes);
  request->attempts_remaining--;
  m_active_da_discovery_timer = m_ss->RegisterSingleTimeout(
      m_config_mc_max,
      NewSingleCallback(this, &SLPServer::ActiveDATick));
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
 * Called when we locate a new DA on the network.
 */
void SLPServer::NewDACallback(const DirectoryAgent &agent) {
  m_ss->RegisterSingleTimeout(
      Random(1000, 3000),
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
}  // slp
}  // ola
