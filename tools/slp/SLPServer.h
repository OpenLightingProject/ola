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
#include <ola/io/SelectServer.h>
#include <ola/io/StdinHandler.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/Socket.h>
#include <ola/network/SocketAddress.h>
#include <ola/network/TCPSocketFactory.h>

#include <memory>
#include <set>
#include <map>
#include <string>
#include <vector>

#include "tools/slp/Base.h"
#include "tools/slp/DATracker.h"
#include "tools/slp/SLPStore.h"
#include "tools/slp/SLPUDPSender.h"
#include "tools/slp/ServerCommon.h"
#include "tools/slp/ServiceEntry.h"
#include "tools/slp/XIDAllocator.h"

using ola::Clock;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::TCPSocket;
using std::auto_ptr;
using std::map;
using std::multimap;
using std::set;
using std::string;

namespace ola {
namespace slp {


class MulicastSrvRqstOperation;
class PendingMulticastOperation;
class PendingSrvRqst;
class UnicastSrvRegOperation;
class UnicastSrvRqstOperation;


/**
 * An SLP Server. This handles the SLP messages an provides an API to
 * register, deregister and locate services.
 */
class SLPServer {
  public:
    struct SLPServerOptions {
      IPV4Address ip_address;  // The interface IP to multicast on
      ola::Clock *clock;  // The clock object to use, if null we create one
      bool enable_da;  // enable the DA mode
      uint16_t slp_port;
      set<string> scopes;  // supported scopes
      // The following come from RFC2608 and are set to their default values
      uint32_t config_da_find;
      uint32_t config_da_beat;  // seconds between DA beats
      uint32_t config_mc_max;
      uint16_t config_retry;
      uint16_t config_retry_max;
      uint16_t config_start_wait;
      uint16_t config_reg_active_min;
      uint16_t config_reg_active_max;
      // The following values are used to initialize the server with a known
      // state for testing.
      uint16_t initial_xid;
      uint32_t boot_time;

      SLPServerOptions();
    };

    SLPServer(ola::io::SelectServerInterface *ss,
              ola::network::UDPSocketInterface *udp_socket,
              ola::network::TCPAcceptingSocket *tcp_socket,
              ola::ExportMap *export_map,
              const SLPServerOptions &options);
    ~SLPServer();

    bool Init();

    const ScopeSet ConfiguredScopes() const { return m_configured_scopes; }

    // bulk load a list of Services
    void DumpStore();
    void GetDirectoryAgents(vector<DirectoryAgent> *output);
    void TriggerActiveDADiscovery();

    // SLP API
    void FindService(const set<string> &scopes,
                     const string &service,
                     BaseCallback1<void, const URLEntries&> *cb);
    uint16_t RegisterService(const ServiceEntry &service);
    uint16_t DeRegisterService(const ServiceEntry &service);

  private:
    typedef multimap<string, class UnicastSrvRegOperation*>
      PendingOperationsByURL;
    typedef SingleUseCallback1<void, uint16_t> AckCallback;
    typedef BaseCallback3<void, const IPV4Address&, uint16_t,
                          const URLEntries&> SrvReplyCallback;
    typedef map<xid_t, AckCallback*> PendingAckMap;
    typedef map<xid_t, SrvReplyCallback*> PendingReplyMap;

    /**
     * A class that cleans up an operation when it goes out of scope.
     */
    class UnicastOperationDeleter {
      public:
        UnicastOperationDeleter(UnicastSrvRegOperation *op, SLPServer *server)
          : op(op),
            server(server) {
        }

        // Cancel the clean up
        void Cancel();

        ~UnicastOperationDeleter();

      private:
        UnicastSrvRegOperation *op;
        SLPServer *server;
    };

    // timing parameters
    uint32_t m_config_da_beat;
    uint32_t m_config_da_find;
    uint32_t m_config_mc_max;
    uint32_t m_config_retry;
    uint32_t m_config_retry_max;
    uint32_t m_config_start_wait;
    uint32_t m_config_reg_active_min;
    uint32_t m_config_reg_active_max;

    bool m_enable_da;
    uint16_t m_slp_port;
    string m_en_lang;
    const IPV4Address m_iface_address;
    ola::TimeStamp m_boot_time;
    const ola::network::IPV4SocketAddress m_multicast_endpoint;
    ola::io::SelectServerInterface *m_ss;
    Clock *m_clock;

    // various timers
    ola::thread::timeout_id m_da_beat_timer;
    ola::thread::timeout_id m_store_cleaner_timer;
    ola::thread::timeout_id m_active_da_discovery_timer;

    // the UDP and TCP sockets for SLP traffic
    ola::network::UDPSocketInterface *m_udp_socket;
    ola::network::TCPAcceptingSocket *m_slp_accept_socket;

    // SLP members
    SLPStore m_service_store;
    SLPUDPSender m_udp_sender;
    ScopeSet m_configured_scopes;
    XIDAllocator m_xid_allocator;

    // Track pending transactions
    // map of xid_t to callbacks to run when we recieve an Ack with this xid.
    PendingAckMap m_pending_acks;
    // multimap url -> PendingOperation for Reg / DeReg operations.
    PendingOperationsByURL m_pending_ops;
    // map of xid_t to callbacks to run when we receive an SrvRply with this
    // xid.
    PendingReplyMap m_pending_replies;

    // Members used to keep track of DAs
    DATracker m_da_tracker;
    auto_ptr<PendingMulticastOperation> m_outstanding_da_discovery;

    // The ExportMap
    ola::ExportMap *m_export_map;

    // SLP Network RX methods
    void UDPData();
    void HandleServiceRequest(BigEndianInputStream *stream,
                              const IPV4SocketAddress &source);
    void HandleServiceReply(BigEndianInputStream *stream,
                            const IPV4SocketAddress &source);
    void HandleServiceRegistration(BigEndianInputStream *stream,
                                   const IPV4SocketAddress &source);
    void HandleServiceDeRegister(BigEndianInputStream *stream,
                                 const IPV4SocketAddress &source);
    void HandleServiceAck(BigEndianInputStream *stream,
                          const IPV4SocketAddress &source);
    void HandleDAAdvert(BigEndianInputStream *stream,
                        const IPV4SocketAddress &source);
    void HandleServiceTypeRequest(BigEndianInputStream *stream,
                                  const IPV4SocketAddress &source);

    // Network TX methods
    void SendErrorIfUnicast(const ServiceRequestPacket *request,
                            slp_function_id_t function_id,
                            const IPV4SocketAddress &source,
                            slp_error_code_t error_code);

    void MaybeSendSAAdvert(const ServiceRequestPacket *request,
                           const IPV4SocketAddress &source);

    void MaybeSendDAAdvert(const ServiceRequestPacket *request,
                           const IPV4SocketAddress &source);

    // DA specific methods
    void SendDAAdvert(const IPV4SocketAddress &dest,
                      uint32_t boot_time,
                      xid_t xid);
    void SendAck(const IPV4SocketAddress &dest, uint16_t error_code);
    bool SendDABeat();

    // UA methods to handle finding services
    void FindServiceInScopes(PendingSrvRqst *request, const ScopeSet &scopes);
    //   methods used to communicate with DAs
    void SendSrvRqstToDA(UnicastSrvRqstOperation *op, const DirectoryAgent &da,
                         bool expect_reused_xid = false);
    void ReceivedDASrvReply(UnicastSrvRqstOperation *op, const IPV4Address &src,
                            uint16_t error_code, const URLEntries &urls);
    void RequestServiceDATimeout(UnicastSrvRqstOperation *op);
    void CancelPendingSrvRqstAck(const PendingReplyMap::iterator &iter);
    //  methods used for multicast SA requests
    void RequestServiceMulticastTimeout(MulicastSrvRqstOperation *op);
    void ReceivedSASrvReply(MulicastSrvRqstOperation *op,
                            const IPV4Address &src, uint16_t error_code,
                            const URLEntries &urls);
    void CheckIfFindSrvComplete(PendingSrvRqst *request);

    // SA methods
    void CancelPendingDAOperationsForService(const string &url);
    void CancelPendingDAOperationsForServiceAndDA(const string &url,
                                                  const string &da_url);
    void FreePendingDAOperation(UnicastSrvRegOperation *op);
    uint16_t InternalRegisterService(const ServiceEntry &service);
    uint16_t InternalDeRegisterService(const ServiceEntry &service);
    void ReceivedAck(UnicastSrvRegOperation *op_ptr, uint16_t error_code);
    void RegistrationTimeout(UnicastSrvRegOperation *op);
    void DeRegistrationTimeout(UnicastSrvRegOperation *op);
    void RegisterWithDA(const DirectoryAgent &directory_agent,
                        const ServiceEntry &service);
    void DeRegisterWithDA(const DirectoryAgent &directory_agent,
                          const ServiceEntry &service);
    void CancelPendingSrvAck(const PendingAckMap::iterator &iter);
    void AddPendingSrvAck(xid_t xid, AckCallback *callback);

    // DA Tracking methods
    void StartActiveDADiscovery();
    void DASrvRqstTimeout();
    void SendDARequestAndSetupTimer(class PendingMulticastOperation *request);
    void ScheduleActiveDADiscovery();
    void NewDACallback(const DirectoryAgent &agent);
    void RegisterServicesWithNewDA(const string da_url);

    // housekeeping
    bool CleanSLPStore();
    void IncrementMethodVar(const string &method);
    void IncrementPacketVar(const string &packet);
    void GetCurrentTime(TimeStamp *time);

    // helper methods
    bool InPRList(const vector<IPV4Address> &pr_list);

    // constants
    /* Super ghetto:
     * Section 6.1 of the RFC says the max UDP data size is 1400, SLP headers
     * are 16 bytes. The lengths of a SrvRqst are another 10, which leaves 137
     * remaining. A PR can be at most 15 bytes.
     * 88 PRs leaves 54 bytes for the service-type & scope strings
     * TODO(simon): fix this for real
     */
    static const unsigned int MAX_PR_LIST_SIZE = 88;
    static const char DAADVERT[];
    static const char DEREGSRVS_ERROR_COUNT_VAR[];
    static const char FINDSRVS_EMPTY_COUNT_VAR[];
    static const char METHOD_CALLS_VAR[];
    static const char METHOD_DEREG_SERVICE[];
    static const char METHOD_FIND_SERVICE[];
    static const char METHOD_REG_SERVICE[];
    static const char REGSRVS_ERROR_COUNT_VAR[];
    static const char SLP_PORT_VAR[];
    static const char SRVACK[];
    static const char SRVDEREG[];
    static const char SRVREG[];
    static const char SRVRPLY[];
    static const char SRVRQST[];
    static const char SRVTYPERQST[];
    static const char UNSUPPORTED[];
    static const char UNKNOWN[];
    static const char UDP_RX_PACKET_BY_TYPE_VAR[];
    static const char UDP_RX_TOTAL_VAR[];
    static const char UDP_TX_PACKET_BY_TYPE_VAR[];
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPSERVER_H_
