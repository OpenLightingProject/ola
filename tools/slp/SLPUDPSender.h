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
 * SLPUDPSender.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_SLPUDPSENDER_H_
#define TOOLS_SLP_SLPUDPSENDER_H_

#include <ola/io/BigEndianStream.h>
#include <ola/io/IOQueue.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/Socket.h>
#include <ola/network/SocketAddress.h>

#include <set>
#include <string>

using ola::network::IPV4SocketAddress;
using std::string;
using std::set;

namespace ola {
namespace slp {

/**
 * The SLPUDPSender constructs packets and sends them on a UDP socket.
 * This class is not thread safe.
 */
class SLPUDPSender {
  public:
    // Ownership is not transferred.
    explicit SLPUDPSender(ola::network::UDPSocketInterface *socket);
    ~SLPUDPSender() {}

    void SendServiceRequest(const IPV4SocketAddress &dest,
                            xid_t xid,
                            const set<IPV4Address> &pr_list,
                            const string &service_type,
                            const ScopeSet &scopes);

    // similar to the above, but doesn't take a PR list, use this if you're
    // unicasting to a DA
    void SendServiceRequest(const IPV4SocketAddress &dest,
                            xid_t xid,
                            const string &service_type,
                            const ScopeSet &scopes);

    void SendServiceReply(const IPV4SocketAddress &dest,
                          xid_t xid,
                          uint16_t error_code,
                          const URLEntries &urls);

    // A Helper method if you just want to send an error
    void SendServiceReply(const IPV4SocketAddress &dest,
                          xid_t xid,
                          uint16_t error_code);

    void SendServiceRegistration(const IPV4SocketAddress &dest,
                                 xid_t xid,
                                 bool fresh,
                                 const ScopeSet &scopes,
                                 const ServiceEntry &service);

    void SendServiceDeRegistration(const IPV4SocketAddress &dest,
                                   xid_t xid,
                                   const ScopeSet &scopes,
                                   const ServiceEntry &service);

    void SendServiceAck(const IPV4SocketAddress &dest,
                        xid_t xid,
                        uint16_t error_code);

    void SendDAAdvert(const IPV4SocketAddress &dest,
                      xid_t xid,
                      uint16_t error_code,
                      uint32_t boot_timestamp,
                      const string &url,
                      const ScopeSet &scopes);

    void SendSAAdvert(const IPV4SocketAddress &dest,
                      xid_t xid,
                      const string &url,
                      const ScopeSet &scopes);

  private:
    ola::network::UDPSocketInterface *m_udp_socket;
    ola::io::IOQueue m_output;
    ola::io::BigEndianOutputStream m_output_stream;
    ola::network::IPV4Address m_multicast_address;

    void EmptyBuffer();
    void Send(const IPV4SocketAddress &dest);
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPUDPSENDER_H_
