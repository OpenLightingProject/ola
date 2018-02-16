/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * NetworkUtils.cpp
 * Abstract various network functions.
 * Copyright (C) 2005 Simon Newton
 */

#include "ola/network/NetworkUtils.h"

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H


#ifdef _WIN32
typedef uint32_t in_addr_t;
// Iphlpapi.h depends on Winsock2.h
#define WIN_32_LEAN_AND_MEAN
#include <ola/win/CleanWinSock2.h>
#include <Iphlpapi.h>
#if HAVE_WINERROR_H
#include <winerror.h>
#endif  // HAVE_WINERROR_H
#else
#include <netinet/in.h>
#endif  // _WIN32

#ifdef HAVE_RESOLV_H
#include <resolv.h>
#endif  // HAVE_RESOLV_H

#if defined(HAVE_LINUX_NETLINK_H) && defined(HAVE_LINUX_RTNETLINK_H)
#define USE_NETLINK_FOR_DEFAULT_ROUTE 1
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#elif defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_NET_ROUTE_H) && \
      defined(HAVE_DECL_PF_ROUTE) && defined(HAVE_DECL_NET_RT_DUMP)
#define USE_SYSCTL_FOR_DEFAULT_ROUTE 1
#include <net/route.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif  // HAVE_SYS_PARAM_H
#include <sys/sysctl.h>
#else
// TODO(Peter): Do something else if we don't have Netlink/on Windows
#endif  // defined(HAVE_LINUX_NETLINK_H) && defined(HAVE_LINUX_RTNETLINK_H)

#ifdef HAVE_ENDIAN_H
#include <endian.h>
#endif  // HAVE_ENDIAN_H
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "common/network/NetworkUtilsInternal.h"
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/math/Random.h"
#include "ola/network/Interface.h"
#include "ola/network/MACAddress.h"
#include "ola/network/SocketCloser.h"


namespace ola {
namespace network {

using std::string;
using std::vector;
using ola::network::Interface;

namespace {

inline bool IsBigEndian() {
#ifdef HAVE_ENDIAN_H
  return BYTE_ORDER == __BIG_ENDIAN;
#else
#ifdef _WIN32
  // Windows currently only runs in little-endian mode, but that might change
  // on future devices. Since there is no BYTE_ORDER define, we use this
  // little trick from http://esr.ibiblio.org/?p=5095
  return (*(uint16_t*)"\0\xff" < 0x100);  // NOLINT(readability/casting)
#else
  return BYTE_ORDER == BIG_ENDIAN;
#endif  // _WIN32
#endif  // HAVE_ENDIAN_H
}

inline uint32_t ByteSwap32(uint32_t value) {
  return ((value & 0x000000ff) << 24) |
         ((value & 0x0000ff00) << 8) |
         ((value & 0x00ff0000) >> 8) |
         ((value & 0xff000000) >> 24);
}

inline uint32_t ByteSwap16(uint16_t value) {
  return ((value & 0xff) << 8) | ((value & 0xff00) >> 8);
}
}  // namespace

unsigned int SockAddrLen(const struct sockaddr &sa) {
#ifdef HAVE_SOCKADDR_SA_LEN
  return sa.sa_len;
#else
  switch (sa.sa_family) {
    case AF_INET:
      return sizeof(struct sockaddr_in);
#ifdef IPV6
    case AF_INET6:
      return sizeof(struct sockaddr_in6);
#endif  // IPV6
#ifdef HAVE_SOCKADDR_DL_STRUCT
    case AF_LINK:
      return sizeof(struct sockaddr_dl);
#endif  // HAVE_SOCKADDR_DL_STRUCT
    default:
      OLA_WARN << "Can't determine size of sockaddr: " << sa.sa_family;
      return sizeof(struct sockaddr);
  }
#endif  // HAVE_SOCKADDR_SA_LEN
}

uint16_t NetworkToHost(uint16_t value) {
  return ntohs(value);
}

uint32_t NetworkToHost(uint32_t value) {
  return ntohl(value);
}

int16_t NetworkToHost(int16_t value) {
  return ntohs(value);
}

int32_t NetworkToHost(int32_t value) {
  return ntohl(value);
}

uint16_t HostToNetwork(uint16_t value) {
  return htons(value);
}

int16_t HostToNetwork(int16_t value) {
  return htons(value);
}

uint32_t HostToNetwork(uint32_t value) {
  return htonl(value);
}

int32_t HostToNetwork(int32_t value) {
  return htonl(value);
}

uint16_t HostToLittleEndian(uint16_t value) {
  if (IsBigEndian()) {
    return ByteSwap16(value);
  } else {
    return value;
  }
}

int16_t HostToLittleEndian(int16_t value) {
  if (IsBigEndian()) {
    return ByteSwap16(value);
  } else {
    return value;
  }
}

uint32_t HostToLittleEndian(uint32_t value) {
  if (IsBigEndian()) {
    return ByteSwap32(value);
  } else {
    return value;
  }
}

int32_t HostToLittleEndian(int32_t value) {
  if (IsBigEndian()) {
    return ByteSwap32(value);
  } else {
    return value;
  }
}

uint16_t LittleEndianToHost(uint16_t value) {
  if (IsBigEndian()) {
    return ByteSwap16(value);
  } else {
    return value;
  }
}


int16_t LittleEndianToHost(int16_t value) {
  if (IsBigEndian()) {
    return ByteSwap16(value);
  } else {
    return value;
  }
}


uint32_t LittleEndianToHost(uint32_t value) {
  if (IsBigEndian()) {
    return ByteSwap32(value);
  } else {
    return value;
  }
}


int32_t LittleEndianToHost(int32_t value) {
  if (IsBigEndian()) {
    return ByteSwap32(value);
  } else {
    return value;
  }
}

string HostnameFromFQDN(const string &fqdn) {
  string::size_type first_dot = fqdn.find_first_of(".");
  if (first_dot == string::npos) {
    return fqdn;
  }
  return fqdn.substr(0, first_dot);  // Don't return the dot itself
}

string DomainNameFromFQDN(const string &fqdn) {
  string::size_type first_dot = string::npos;
  first_dot = fqdn.find_first_of(".");
  if (first_dot == string::npos) {
    return "";
  }
  return fqdn.substr(first_dot + 1);  // Don't return the dot itself
}


string DomainName() {
  return DomainNameFromFQDN(FQDN());
}


string FQDN() {
#ifdef _POSIX_HOST_NAME_MAX
  char hostname[_POSIX_HOST_NAME_MAX];
#else
  char hostname[256];
#endif  // _POSIX_HOST_NAME_MAX
  int ret = gethostname(hostname, sizeof(hostname));

  if (ret) {
    OLA_WARN << "gethostname failed: " << strerror(errno);
    return "";
  }
  return hostname;
}


string FullHostname() {
  return FQDN();
}


string Hostname() {
  return HostnameFromFQDN(FQDN());
}


bool NameServers(vector<IPV4Address> *name_servers) {
#if HAVE_DECL_RES_NINIT
  struct __res_state res;
  memset(&res, 0, sizeof(struct __res_state));

  // Init the resolver info each time so it's always current for the RDM
  // responders in case we've set it via RDM too
  if (res_ninit(&res) != 0) {
    OLA_WARN << "Error getting nameservers via res_ninit";
    return false;
  }

  for (int32_t i = 0; i < res.nscount; i++) {
    IPV4Address addr = IPV4Address(res.nsaddr_list[i].sin_addr.s_addr);
    OLA_DEBUG << "Found Nameserver " << i << ": " << addr;
    name_servers->push_back(addr);
  }

  res_nclose(&res);
#elif defined(_WIN32)
  ULONG size = sizeof(FIXED_INFO);
  PFIXED_INFO fixed_info = NULL;
  while (1) {
    fixed_info = reinterpret_cast<PFIXED_INFO>(new uint8_t[size]);
    DWORD result = GetNetworkParams(fixed_info, &size);
    if (result == ERROR_SUCCESS) {
      break;
    }

    if (result != ERROR_BUFFER_OVERFLOW) {
      OLA_WARN << "GetNetworkParams failed with: " << GetLastError();
      return false;
    }

    delete[] fixed_info;
  }

  IP_ADDR_STRING* addr = &(fixed_info->DnsServerList);
  for (; addr; addr = addr->Next) {
    IPV4Address ipv4addr = IPV4Address(inet_addr(addr->IpAddress.String));
    OLA_DEBUG << "Found nameserver: " << ipv4addr;
    name_servers->push_back(ipv4addr);
  }

  delete[] fixed_info;
#else
  // Init the resolver info each time so it's always current for the RDM
  // responders in case we've set it via RDM too
  if (res_init() != 0) {
    OLA_WARN << "Error getting nameservers via res_init";
    return false;
  }

  for (int32_t i = 0; i < _res.nscount; i++) {
    IPV4Address addr = IPV4Address(_res.nsaddr_list[i].sin_addr.s_addr);
    OLA_DEBUG << "Found Nameserver " << i << ": " << addr;
    name_servers->push_back(addr);
  }
#endif  // HAVE_DECL_RES_NINIT

  return true;
}


#ifdef USE_SYSCTL_FOR_DEFAULT_ROUTE

/**
 * Try to extract an AF_INET address from a sockaddr. If successful, sa points
 * to the next sockaddr and true is returned.
 */
bool ExtractIPV4AddressFromSockAddr(const uint8_t **data,
                                    IPV4Address *ip) {
  const struct sockaddr *sa = reinterpret_cast<const struct sockaddr*>(*data);
  if (sa->sa_family != AF_INET) {
    return false;
  }

  *ip = IPV4Address(
      reinterpret_cast<const struct sockaddr_in*>(*data)->sin_addr.s_addr);
  *data += SockAddrLen(*sa);
  return true;
}

/**
 * Use sysctl() to get the default route
 */
static bool GetDefaultRouteWithSysctl(int32_t *if_index,
                                      IPV4Address *default_gateway) {
  int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_DUMP, 0};

  size_t space_required;
  uint8_t *buffer = NULL;
  // loop until we know we've read all the data.
  while (1) {
    int ret = sysctl(mib, 6, NULL, &space_required, NULL, 0);

    if (ret < 0) {
      OLA_WARN << "sysctl({CTL_NET, PF_ROUTE, 0, 0, NET_RT_DUMP, 0}, 6, NULL) "
               << "failed: " << strerror(errno);
      return false;
    }
    buffer = new uint8_t[space_required];

    ret = sysctl(mib, 6, buffer, &space_required, NULL, 0);
    if (ret < 0) {
      delete[] buffer;
      if (errno == ENOMEM) {
        continue;
      } else {
        OLA_WARN
            << "sysctl({CTL_NET, PF_ROUTE, 0, 0, NET_RT_DUMP, 0}, 6, !NULL)"
            << " failed: " << strerror(errno);
        return false;
      }
    } else {
      break;
    }
  }

  const struct rt_msghdr *rtm = NULL;
  const uint8_t *end = buffer + space_required;

  for (const uint8_t *next = buffer; next < end; next += rtm->rtm_msglen) {
    rtm = reinterpret_cast<const struct rt_msghdr*>(next);
    if (rtm->rtm_version != RTM_VERSION) {
      OLA_WARN << "Old RTM_VERSION, was " << rtm->rtm_version << ", expected "
               << RTM_VERSION;
      continue;
    }

    const uint8_t *data_start = reinterpret_cast<const uint8_t*>(rtm + 1);

    IPV4Address dest, gateway, netmask;

    if (rtm->rtm_flags & RTA_DST) {
      if (!ExtractIPV4AddressFromSockAddr(&data_start, &dest)) {
        continue;
      }
    }

    if (rtm->rtm_flags & RTA_GATEWAY) {
      if (!ExtractIPV4AddressFromSockAddr(&data_start, &gateway)) {
        continue;
      }
    }

    if (rtm->rtm_flags & RTA_NETMASK) {
      if (!ExtractIPV4AddressFromSockAddr(&data_start, &netmask)) {
        continue;
      }
    }

    if (dest.IsWildcard() && netmask.IsWildcard()) {
      *default_gateway = gateway;
      *if_index = rtm->rtm_index;
      delete[] buffer;
      OLA_INFO << "Default gateway: " << *default_gateway << ", if_index: "
               << *if_index;
      return true;
    }
  }
  delete[] buffer;
  OLA_WARN << "No default route found";
  return true;
}
#elif defined(USE_NETLINK_FOR_DEFAULT_ROUTE)

/**
 * Handle a netlink message. If this message is a routing table message and it
 * contains the default route, then either:
 *   i) default_gateway is updated with the address of the gateway.
 *   ii) if_index is updated with the interface index for the default route.
 * @param if_index[out] possibly updated with interface index for the default
 *   route.
 * @param default_gateway[out] possibly updated with the default gateway.
 * @param nl_hdr the netlink message.
 */
void MessageHandler(int32_t *if_index,
                    IPV4Address *default_gateway,
                    const struct nlmsghdr *nl_hdr) {
  // Unless RTA_DST is provided, an RTA_GATEWAY or RTA_OIF attribute implies
  // it's the default route.
  IPV4Address gateway;
  int32_t index = Interface::DEFAULT_INDEX;

  bool is_default_route = true;

  // Loop over the attributes looking for RTA_GATEWAY and/or RTA_DST
  const rtmsg *rt_msg = reinterpret_cast<const rtmsg*>(NLMSG_DATA(nl_hdr));
  if (rt_msg->rtm_family == AF_INET && rt_msg->rtm_table == RT_TABLE_MAIN) {
    int rt_len = RTM_PAYLOAD(nl_hdr);

    for (const rtattr* rt_attr = reinterpret_cast<const rtattr*>(
            RTM_RTA(rt_msg));
         RTA_OK(rt_attr, rt_len);
         rt_attr = RTA_NEXT(rt_attr, rt_len)) {
      switch (rt_attr->rta_type) {
        case RTA_OIF:
          index = *(reinterpret_cast<int32_t*>(RTA_DATA(rt_attr)));
          break;
        case RTA_GATEWAY:
          gateway = IPV4Address(
              reinterpret_cast<const in_addr*>(RTA_DATA(rt_attr))->s_addr);
          break;
        case RTA_DST:
          IPV4Address dest(
              reinterpret_cast<const in_addr*>(RTA_DATA(rt_attr))->s_addr);
          is_default_route = dest.IsWildcard();
          break;
      }
    }
  }

  if (is_default_route &&
      (!gateway.IsWildcard() || index != Interface::DEFAULT_INDEX)) {
    *default_gateway = gateway;
    *if_index = index;
  }
}

typedef ola::Callback1<void, const struct nlmsghdr*> NetlinkCallback;

/**
 * Read a message from the netlink socket. This continues to read until the
 * expected sequence number is seend. Returns true if the desired message was
 * seen, false if there was an error reading from the netlink socket.
 */
bool ReadNetlinkSocket(int sd, uint8_t *buffer, int bufsize, unsigned int seq,
                       NetlinkCallback *handler) {
  OLA_DEBUG << "Looking for netlink response with seq: " << seq;
  while (true) {
    int len = recv(sd, buffer, bufsize, 0);
    if (len < 0) {
      return false;
    }
    if (len == static_cast<int>(bufsize)) {
      OLA_WARN << "Number of bytes fetched == buffer size ("
               << bufsize << "), Netlink data may be truncated";
    }

    struct nlmsghdr* nl_hdr;
    for (nl_hdr = reinterpret_cast<struct nlmsghdr*>(buffer);
         NLMSG_OK(nl_hdr, static_cast<unsigned int>(len));
         nl_hdr = NLMSG_NEXT(nl_hdr, len)) {
      OLA_DEBUG << "Read seq " << nl_hdr->nlmsg_seq << ", pid "
                << nl_hdr->nlmsg_pid << ", type "
                << nl_hdr->nlmsg_type << ", from netlink socket";

      if (static_cast<unsigned int>(nl_hdr->nlmsg_seq) != seq) {
        continue;
      }

      if (nl_hdr->nlmsg_type == NLMSG_ERROR) {
        struct nlmsgerr* err = reinterpret_cast<struct nlmsgerr*>(
            NLMSG_DATA(nl_hdr));
        OLA_WARN << "Netlink returned error: " << err->error;
        return false;
      }

      handler->Run(nl_hdr);
      if ((nl_hdr->nlmsg_flags & NLM_F_MULTI) == 0 ||
          nl_hdr->nlmsg_type == NLMSG_DONE) {
        return true;
      }
    }
  }
}

/**
 * Get the default route using a netlink socket
 */
static bool GetDefaultRouteWithNetlink(int32_t *if_index,
                                       IPV4Address *default_gateway) {
  int sd = socket(PF_ROUTE, SOCK_DGRAM, NETLINK_ROUTE);
  if (sd < 0) {
    OLA_WARN << "Could not create Netlink socket " << strerror(errno);
    return false;
  }
  SocketCloser closer(sd);

  int seq = ola::math::Random(0, INT_MAX);

  const unsigned int BUFSIZE = 8192;
  uint8_t msg[BUFSIZE];
  memset(msg, 0, BUFSIZE);

  nlmsghdr* nl_msg = reinterpret_cast<nlmsghdr*>(msg);
  nl_msg->nlmsg_len = NLMSG_LENGTH(sizeof(rtmsg));
  nl_msg->nlmsg_type = RTM_GETROUTE;
  nl_msg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
  nl_msg->nlmsg_seq = seq++;
  nl_msg->nlmsg_pid = 0;

  if (send(sd, nl_msg, nl_msg->nlmsg_len, 0) < 0) {
    OLA_WARN << "Could not send data to Netlink " << strerror(errno);
    return false;
  }

  std::auto_ptr<NetlinkCallback> cb(
      ola::NewCallback(MessageHandler, if_index, default_gateway));
  if (!ReadNetlinkSocket(sd, msg, BUFSIZE, nl_msg->nlmsg_seq, cb.get())) {
    return false;
  }

  if (default_gateway->IsWildcard() && *if_index == Interface::DEFAULT_INDEX) {
    OLA_WARN << "No default route found";
  }
  OLA_INFO << "Default gateway: " << *default_gateway << ", if_index: "
           << *if_index;
  return true;
}
#endif  // USE_SYSCTL_FOR_DEFAULT_ROUTE

bool DefaultRoute(int32_t *if_index, IPV4Address *default_gateway) {
  *default_gateway = IPV4Address();
  *if_index = Interface::DEFAULT_INDEX;
#ifdef USE_SYSCTL_FOR_DEFAULT_ROUTE
  return GetDefaultRouteWithSysctl(if_index, default_gateway);
#elif defined(USE_NETLINK_FOR_DEFAULT_ROUTE)
  return GetDefaultRouteWithNetlink(if_index, default_gateway);
#elif defined(_WIN32)
  ULONG size = 4096;
  PMIB_IPFORWARDTABLE forward_table =
      reinterpret_cast<PMIB_IPFORWARDTABLE>(malloc(size));
  DWORD result = GetIpForwardTable(forward_table, &size, TRUE);
  if (result == NO_ERROR) {
    for (unsigned int i = 0; i < forward_table->dwNumEntries; ++i) {
      if (forward_table->table[i].dwForwardDest == 0) {
        *default_gateway =
            IPV4Address(forward_table->table[i].dwForwardNextHop);
        *if_index = forward_table->table[i].dwForwardIfIndex;
      }
    }
    free(forward_table);
    return true;
  } else {
    OLA_WARN << "GetIpForwardTable failed with " << GetLastError();
    return false;
  }
#else
#error "DefaultRoute not implemented for this platform, please report this."
  // TODO(Peter): Do something else on machines without Netlink
  // No Netlink, can't do anything
  return false;
#endif  // USE_SYSCTL_FOR_DEFAULT_ROUTE
}
}  // namespace network
}  // namespace ola
