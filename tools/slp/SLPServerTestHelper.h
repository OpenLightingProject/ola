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
 * SLPServerTestHelper.h
 * A Helper class for the SLPServer tests.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_SLPSERVERTESTHELPER_H_
#define TOOLS_SLP_SLPSERVERTESTHELPER_H_

#include <stdint.h>
#include <cppunit/extensions/HelperMacros.h>
#include <set>
#include <string>
#include "ola/Clock.h"
#include "ola/io/SelectServer.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/SocketAddress.h"
#include "ola/testing/MockUDPSocket.h"
#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/SLPServer.h"
#include "tools/slp/ScopeSet.h"
#include "tools/slp/ServiceEntry.h"
#include "tools/slp/URLEntry.h"

using ola::io::SelectServer;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::slp::SLPServer;
using ola::slp::ScopeSet;
using ola::slp::ServiceEntry;
using ola::slp::URLEntries;
using ola::slp::slp_function_id_t;
using ola::slp::xid_t;
using ola::testing::MockUDPSocket;
using std::set;
using std::string;


/**
 * This objects takes a MockUDPSocket and allows one to either expect SLP
 * messages, or inject SLP messages. See MockUDPSocket for more info on expect
 * vs inject.
 */
class SLPServerTestHelper {
  public:
    explicit SLPServerTestHelper(MockUDPSocket *mock_socket)
        : m_ss(NULL, &m_clock),
          m_udp_socket(mock_socket),
          m_output_stream(&m_output) {
    }


    // Advance the time, which may trigger timeouts to run
    void AdvanceTime(int32_t sec, int32_t usec) {
      m_clock.AdvanceTime(sec, usec);
      // run any timeouts, and update the WakeUpTime
      m_ss.RunOnce(0, 0);
    }

    void RunOnce() {
      m_ss.RunOnce(0, 0);
    }

    // Print the time since the server started, this is useful for debugging
    void PrintTimePassed() {
      ola::TimeStamp now = *m_ss.WakeUpTime();
      OLA_INFO << "Now " << now << ", delta from start is "
               << (now - m_server_start_time);
    }

    SLPServer *CreateNewServer(bool enable_da, const string &scopes);
    void HandleInitialActiveDADiscovery(const string &scopes);
    void HandleActiveDADiscovery(const string &scopes, xid_t xid);
    void RegisterWithDA(SLPServer *server,
                        const IPV4SocketAddress &da_addr,
                        const ServiceEntry &service,
                        xid_t xid);

    void InjectServiceRequest(const IPV4SocketAddress &source,
                              xid_t xid,
                              bool multicast,
                              const set<IPV4Address> &pr_list,
                              const string &service_type,
                              const ScopeSet &scopes);
    void InjectSrvAck(const IPV4SocketAddress &source,
                      xid_t xid,
                      uint16_t error_code);
    void InjectDAAdvert(const IPV4SocketAddress &source,
                        xid_t xid,
                        bool multicast,
                        uint16_t error_code,
                        uint32_t boot_timestamp,
                        const ScopeSet &scopes);
    void InjectError(const IPV4SocketAddress &source,
                     slp_function_id_t function_id,
                     xid_t xid,
                     uint16_t error_code);

    void ExpectServiceReply(const IPV4SocketAddress &dest,
                            xid_t xid,
                            uint16_t error_code,
                            const URLEntries &urls);
    void ExpectDAServiceRequest(xid_t xid,
                                const set<IPV4Address> &pr_list,
                                const ScopeSet &scopes);
    void ExpectServiceRegistration(const IPV4SocketAddress &dest,
                                   xid_t xid,
                                   bool fresh,
                                   const ScopeSet &scopes,
                                   const ServiceEntry &service);
    void ExpectServiceDeRegistration(const IPV4SocketAddress &dest,
                                     xid_t xid,
                                     const ScopeSet &scopes,
                                     const ServiceEntry &service);
    void ExpectDAAdvert(const IPV4SocketAddress &dest,
                        xid_t xid,
                        bool multicast,
                        uint16_t error_code,
                        uint32_t boot_timestamp,
                        const ScopeSet &scopes);
    void ExpectMulticastDAAdvert(xid_t xid,
                                 uint32_t boot_timestamp,
                                 const ScopeSet &scopes);
    void ExpectSAAdvert(const IPV4SocketAddress &dest,
                        xid_t xid,
                        const ScopeSet &scopes);

    void ExpectError(const IPV4SocketAddress &dest,
                     slp_function_id_t function_id,
                     xid_t xid,
                     uint16_t error_code);

    void VerifyKnownDAs(unsigned int line,
                        SLPServer *server,
                        const set<IPV4Address> &expected_das);

    static const uint16_t SLP_TEST_PORT = 5570;
    static const uint32_t INITIAL_BOOT_TIME = 12345;
    static const char SERVER_IP[];
    static const char SLP_MULTICAST_IP[];

  private:
    ola::MockClock m_clock;
    ola::TimeStamp m_server_start_time;
    ola::io::SelectServer m_ss;
    MockUDPSocket *m_udp_socket;
    ola::io::IOQueue m_output;
    ola::io::BigEndianOutputStream m_output_stream;
};
#endif  // TOOLS_SLP_SLPSERVERTESTHELPER_H_
