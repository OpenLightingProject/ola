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
#include <sstream>
#include <string>
#include <vector>
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/network/Interface.h"
#include "ola/network/MACAddress.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SocketCloser.h"


namespace ola {
namespace network {

using std::string;
using std::vector;

unsigned int SockAddrLen(const struct sockaddr &sa) {
#ifdef HAVE_SOCKADDR_SA_LEN
  return sa.sa_len;
#else
  unsigned int socket_len = sizeof(struct sockaddr);
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

#ifdef USE_NETLINK_FOR_DEFAULT_ROUTE
int ReadNetlinkSocket(int sd, char *buf, int bufsize, int seq, int pid) {
  nlmsghdr* nl_hdr;

  unsigned int msglen = 0;

  do {
    int readlen = recv(sd, buf, bufsize - msglen, 0);
    if (readlen < 0) return -1;

    nl_hdr = reinterpret_cast<nlmsghdr*>(buf);

    // We have to convert the type of the NLMSG_OK length, as otherwise it
    // generates "comparison between signed and unsigned integer expressions"
    // errors
    if (NLMSG_OK(nl_hdr, (unsigned int)readlen) == 0)
      return -1;

    if (nl_hdr->nlmsg_type == NLMSG_ERROR)
      return -1;

    if (nl_hdr->nlmsg_type == NLMSG_DONE)
      break;

    buf += readlen;
    msglen += readlen;

    if ((nl_hdr->nlmsg_flags & NLM_F_MULTI) == 0) break;
  } while ((static_cast<int>(nl_hdr->nlmsg_seq) != seq) ||
           (static_cast<int>(nl_hdr->nlmsg_pid) != pid));

  return msglen;
}
#endif

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
      OLA_INFO << "Default route is " << dest << ", gw " << gateway << ", mask "
               << netmask;
      *default_route = dest;
      free(buffer);
      return true;
    }
  }
  free(buffer);
  return false;
}
#endif

bool DefaultRoute(IPV4Address *default_route) {
#ifdef USE_SYSCTL_FOR_DEFAULT_ROUTE
  return GetDefaultRouteWithSysctl(default_route);
#elif defined(USE_NETLINK_FOR_DEFAULT_ROUTE)
  OLA_INFO << "Getting default route";

  static const unsigned int BUFSIZE = 8192;

  int sd = socket(PF_ROUTE, SOCK_DGRAM, NETLINK_ROUTE);
  if (sd < 0) {
    OLA_WARN << "Could not create Netlink socket " << strerror(errno);
    return false;
  }

  SocketCloser closer(sd);

  int seq = 0;
  // TODO(Peter): Fix me when linking behaves: ola::math::Random(0, INT_MAX);

  char msg[BUFSIZE];
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

  int len = 0;

  // nlmsghdr* nl_hdr;

  // TODO(Peter): Switch to the inline code below when we can get
  // *msg += readlen; working properly.
  len = ReadNetlinkSocket(sd,
                          msg,
                          BUFSIZE,
                          nl_msg->nlmsg_seq,
                          nl_msg->nlmsg_pid);
  if (len == static_cast<int>(BUFSIZE)) {
    OLA_WARN << "Number of bytes fetched == buffer size (" << BUFSIZE << "), "
                "Netlink data may be truncated";
  }

 /*
  * do {
  *   OLA_WARN << "Looping len: " << len;
  *
  *
  *   int readlen = recv(sd, &msg, BUFSIZE - len, 0);
  *   if (readlen < 0) {
  *     len = -1;
  *     break;
  *   }
  *
  *
  *   nl_hdr = (nlmsghdr*)msg;
  *
  *
  *   // We have to convert the type of the NLMSG_OK length, as otherwise it
  *   // generates "comparison between signed and unsigned integer expressions"
  *   // errors
  *   if (NLMSG_OK(nl_hdr, (unsigned int)readlen) == 0) {
  *     OLA_WARN << "nlmsg ok";
  *     len = -1;
  *     break;
  *   }
  *
  *
  *   if (nl_hdr->nlmsg_type == NLMSG_ERROR) {
  *     OLA_WARN << "nlmsg err";
  *     len = -1;
  *     break;
  *   }
  *
  *
  *   if (nl_hdr->nlmsg_type == NLMSG_DONE) {
  *     OLA_WARN << "nlmsg err";
  *     break;
  *   }
  *
  *
  *   *msg += readlen;
  *   len += readlen;
  *
  *
  * OLA_WARN << "Postinc len: " << len;

  * if ((nl_hdr->nlmsg_flags & NLM_F_MULTI) == 0)
  *   break;
  *} while (((int)nl_hdr->nlmsg_seq != seq) ||
  *        ((int)nl_hdr->nlmsg_pid != 0));
 */

  if (len < 0) {
    OLA_WARN << "No data received from Netlink " << strerror(errno);
    return false;
  }

  unsigned int route_count = 0;
  bool found_default_route = false;
  in_addr default_route_ip = {0};

  // We have to convert the type of the NLMSG_OK length, as otherwise it
  // generates "comparison between signed and unsigned integer expressions"
  // errors
  for (;
       NLMSG_OK(nl_msg, (unsigned int)len);
       nl_msg = NLMSG_NEXT(nl_msg, len)) {
    if (nl_msg->nlmsg_type == NLMSG_DONE) {
      break;
    }

    rtmsg* rt_msg = reinterpret_cast<rtmsg*>(NLMSG_DATA(nl_msg));

    found_default_route = false;

    OLA_WARN << "Checking msg";

    if ((rt_msg->rtm_family == AF_INET) &&
        (rt_msg->rtm_table == RT_TABLE_MAIN)) {
      int rt_len = RTM_PAYLOAD(nl_msg);

      for (rtattr* rt_attr = reinterpret_cast<rtattr*>(RTM_RTA(rt_msg));
           RTA_OK(rt_attr, rt_len);
           rt_attr = RTA_NEXT(rt_attr, rt_len)) {
        OLA_WARN << "Checking attr " << static_cast<int>(rt_attr->rta_type);
        switch (rt_attr->rta_type) {
          case RTA_OIF:
            OLA_WARN << "Index: " <<
                *(reinterpret_cast<int*>(RTA_DATA(rt_attr)));
            route_count++;
            break;
          case RTA_GATEWAY:
            OLA_WARN << "GW: " <<
                static_cast<int>(
                    (reinterpret_cast<in_addr*>(RTA_DATA(rt_attr)))->s_addr) <<
                " = " << IPV4Address(
                    (reinterpret_cast<in_addr*>(RTA_DATA(rt_attr)))->s_addr);
            default_route_ip.s_addr = reinterpret_cast<in_addr*>(
                RTA_DATA(rt_attr))->s_addr;
            found_default_route = true;
            break;
          case RTA_DST:
            if ((reinterpret_cast<in_addr*>(RTA_DATA(rt_attr)))->s_addr == 0) {
              OLA_WARN << "Default GW:";
            }
            OLA_WARN << "Dest: " <<
                static_cast<int>(
                    (reinterpret_cast<in_addr*>(RTA_DATA(rt_attr)))->s_addr) <<
                " = " << IPV4Address(
                    (reinterpret_cast<in_addr*>(RTA_DATA(rt_attr)))->s_addr);
            break;
        }
      }
      OLA_WARN << "====================================";
      if (found_default_route)
        break;
    }
  }

  OLA_DEBUG << "Found " << route_count << " routes";

  if (!found_default_route) {
    if (route_count > 0) {
      OLA_WARN << "No default route found, but found " << route_count
               << " routes, so setting default route to zero";
      default_route_ip.s_addr = 0;
    } else {
      OLA_WARN << "Couldn't find default route";
      return false;
    }
  }

  *default_route = IPV4Address(default_route_ip.s_addr);

  OLA_INFO << "Got default route: " << *default_route;

  return true;
#else
  // TODO(Peter): Do something else on Windows/machines without Netlink
  // No Netlink, can't do anything
  (void) default_route;

  return false;
#endif
}
}  // namespace network
}  // namespace ola
