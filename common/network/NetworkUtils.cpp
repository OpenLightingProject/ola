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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NetworkUtils.cpp
 * Abstract various network functions.
 * Copyright (C) 2005-2014 Simon Newton
 * Default Route code based on code by Arvid Norberg from:
 * http://code.google.com/p/libtorrent/source/browse/src/enum_net.cpp
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef WIN32
typedef uint32_t in_addr_t;
#else
#include <resolv.h>
#endif

#if defined(HAVE_LINUX_NETLINK_H) && defined(HAVE_LINUX_RTNETLINK_H)
#define USE_NETLINK_FOR_DEFAULT_ROUTE 1
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#elif defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_NET_ROUTE_H) && \
      defined(HAVE_DECL_PF_ROUTE) && defined(HAVE_DECL_NET_RT_DUMP)
#define USE_SYSCTL_FOR_DEFAULT_ROUTE 1
#include <net/route.h>
#include <sys/sysctl.h>
#else
// Do something else if we don't have Netlink/on Windows
#endif

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/math/Random.h"
#include "ola/network/Interface.h"
#include "ola/network/MACAddress.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SocketCloser.h"
#include "ola/StringUtils.h"


namespace ola {
namespace network {

using std::string;
using std::vector;

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
#endif
#ifdef HAVE_SOCKADDR_DL_STRUCT
    case AF_LINK:
      return sizeof(struct sockaddr_dl);
#endif
    default:
      OLA_WARN << "Can't determine size of sockaddr: " << sa.sa_family;
      return sizeof(struct sockaddr);
  }
#endif
}


bool StringToAddress(const string &address, struct in_addr *addr) {
  bool ok;

#ifdef HAVE_INET_ATON
  ok = (1 == inet_aton(address.data(), addr));
#else
  in_addr_t ip_addr4 = inet_addr(address.c_str());
  ok = (INADDR_NONE != ip_addr4 || address == "255.255.255.255");
  addr->s_addr = ip_addr4;
#endif

  if (!ok) {
    OLA_WARN << "Could not convert address " << address;
  }
  return ok;
}


string AddressToString(const struct in_addr &addr) {
  return inet_ntoa(addr);
}


bool IsBigEndian() {
#ifdef HAVE_ENDIAN_H
  return BYTE_ORDER == __BIG_ENDIAN;
#else
  return BYTE_ORDER == BIG_ENDIAN;
#endif
}


uint8_t NetworkToHost(uint8_t value) {
  return value;
}


uint16_t NetworkToHost(uint16_t value) {
  return ntohs(value);
}


uint32_t NetworkToHost(uint32_t value) {
  return ntohl(value);
}


int8_t NetworkToHost(int8_t value) {
  return value;
}


int16_t NetworkToHost(int16_t value) {
  return ntohs(value);
}


int32_t NetworkToHost(int32_t value) {
  return ntohl(value);
}


uint8_t HostToNetwork(uint8_t value) {
  return value;
}


int8_t HostToNetwork(int8_t value) {
  return value;
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


uint8_t HostToLittleEndian(uint8_t value) {
  return value;
}


int8_t HostToLittleEndian(int8_t value) {
  return value;
}


uint16_t HostToLittleEndian(uint16_t value) {
  if (IsBigEndian())
    return ((value & 0xff) << 8) | (value >> 8);
  else
    return value;
}


int16_t HostToLittleEndian(int16_t value) {
  if (IsBigEndian())
    return ((value & 0xff) << 8) | (value >> 8);
  else
    return value;
}


uint32_t _ByteSwap(uint32_t value) {
  return ((value & 0x000000ff) << 24) |
         ((value & 0x0000ff00) << 8) |
         ((value & 0x00ff0000) >> 8) |
         ((value & 0xff000000) >> 24);
}

uint32_t HostToLittleEndian(uint32_t value) {
  if (IsBigEndian())
    return _ByteSwap(value);
  else
    return value;
}


int32_t HostToLittleEndian(int32_t value) {
  if (IsBigEndian())
    return _ByteSwap(value);
  else
    return value;
}


uint8_t LittleEndianToHost(uint8_t value) {
  return value;
}


int8_t LittleEndianToHost(int8_t value) {
  return value;
}


uint16_t LittleEndianToHost(uint16_t value) {
  if (IsBigEndian())
    return ((value & 0xff) << 8) | (value >> 8);
  else
    return value;
}


int16_t LittleEndianToHost(int16_t value) {
  if (IsBigEndian())
    return ((value & 0xff) << 8) | (value >> 8);
  else
    return value;
}


uint32_t LittleEndianToHost(uint32_t value) {
  if (IsBigEndian())
    return _ByteSwap(value);
  else
    return value;
}


int32_t LittleEndianToHost(int32_t value) {
  if (IsBigEndian())
    return _ByteSwap(value);
  else
    return value;
}


string HostnameFromFQDN(const string &fqdn) {
  string::size_type first_dot = fqdn.find_first_of(".");
  if (first_dot == string::npos)
    return fqdn;
  return fqdn.substr(0, first_dot);  // Don't return the dot itself
}


string DomainNameFromFQDN(const string &fqdn) {
  string::size_type first_dot = string::npos;
  first_dot = fqdn.find_first_of(".");
  if (first_dot == string::npos)
    return "";
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
#endif
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
  // TODO(Peter): Do something on Windows

  // Init the resolver info each time so it's always current for the RDM
  // responders in case we've set it via RDM too
  if (res_init() != 0) {
    OLA_WARN << "Error getting nameservers";
    return false;
  }

  for (int32_t i = 0; i < _res.nscount; i++) {
    IPV4Address addr = IPV4Address(_res.nsaddr_list[i].sin_addr);
    OLA_DEBUG << "Found Nameserver " << i << ": " << addr;
    name_servers->push_back(addr);
  }

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
      reinterpret_cast<const struct sockaddr_in*>(*data)->sin_addr);
  *data += SockAddrLen(*sa);
  return true;
}

/**
 * Use sysctl() to get the default route
 */
static bool GetDefaultRouteWithSysctl(IPV4Address *default_route) {
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
    buffer = reinterpret_cast<uint8_t*>(malloc(space_required));

    ret = sysctl(mib, 6, buffer, &space_required, NULL, 0);
    if (ret < 0) {
      free(buffer);
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
      *default_route = dest;
      free(buffer);
      return true;
    }
  }
  free(buffer);
  return false;
}
#elif defined(USE_NETLINK_FOR_DEFAULT_ROUTE)

/**
 * Handle a netlink message. If this message is a routing table message and it
 * contains the default route, then default_route is updated with the address
 * of the gateway.
 * @param default_route[out] possibly updated with the default gateway.
 * @param nl_hdr the netlink message.
 */
void MessageHandler(IPV4Address *default_route,
                    const struct nlmsghdr *nl_hdr) {
  // Unless RTA_DST is provided, an RTA_GATEWAY attribute implies it's the
  // default route.
  IPV4Address gateway;
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
        case RTA_GATEWAY:
          gateway = IPV4Address(
              *(reinterpret_cast<const in_addr*>(RTA_DATA(rt_attr))));
          break;
        case RTA_DST:
          IPV4Address dest(*(reinterpret_cast<const in_addr*>(
              RTA_DATA(rt_attr))));
          is_default_route = dest.IsWildcard();
          break;
      }
    }
  }

  if (is_default_route && !gateway.IsWildcard()) {
    *default_route = gateway;
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
static bool GetDefaultRouteWithNetlink(IPV4Address *default_route) {
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

  *default_route = IPV4Address();
  std::auto_ptr<NetlinkCallback> cb(
      ola::NewCallback(MessageHandler, default_route));
  bool ok = ReadNetlinkSocket(sd, msg, BUFSIZE, nl_msg->nlmsg_seq, cb.get());
  if (!ok) {
    return false;
  }

  if (default_route->IsWildcard()) {
    OLA_WARN << "No default route found, so setting default route to 0.0.0.0";
  }
  OLA_INFO << "Default route is " << *default_route;
  return ok;
}
#endif

bool DefaultRoute(IPV4Address *default_route) {
#ifdef USE_SYSCTL_FOR_DEFAULT_ROUTE
  return GetDefaultRouteWithSysctl(default_route);
#elif defined(USE_NETLINK_FOR_DEFAULT_ROUTE)
  return GetDefaultRouteWithNetlink(default_route);
#else
#error DefaultRoute not implemented
  // TODO(Peter): Do something else on Windows/machines without Netlink
  // No Netlink, can't do anything
  return false;
#endif
}
}  // namespace network
}  // namespace ola
