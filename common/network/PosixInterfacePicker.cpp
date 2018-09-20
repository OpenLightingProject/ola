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
 * PosixInterfacePicker.cpp
 * Chooses an interface to listen on
 * Copyright (C) 2005 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#ifdef HAVE_GETIFADDRS
  #ifdef HAVE_LINUX_IF_PACKET_H
    #include <ifaddrs.h>
    #include <linux/types.h>
    #include <linux/if_packet.h>
  #endif  // HAVE_LINUX_IF_PACKET_H
#endif  // HAVE_GETIFADDRS

#ifdef HAVE_SYS_TYPES_H
  #include <sys/types.h>  // Required by OpenBSD
#endif  // HAVE_SYS_TYPES_H
#ifdef HAVE_SYS_SOCKET_H
  #include <sys/socket.h>  // order is important for FreeBSD
#endif  // HAVE_SYS_SOCKET_H
#include <arpa/inet.h>
#ifdef HAVE_NETINET_IN_H
  #include <netinet/in.h>  // Required by FreeBSD
#endif  // HAVE_NETINET_IN_H
#include <errno.h>
#include <net/if.h>
#ifdef HAVE_SOCKADDR_DL_STRUCT
  #include <net/if_dl.h>
#endif  // HAVE_SOCKADDR_DL_STRUCT
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <algorithm>
#include <string>
#include <vector>

#include "common/network/NetworkUtilsInternal.h"
#include "common/network/PosixInterfacePicker.h"
#include "ola/Logging.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/MACAddress.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SocketCloser.h"

namespace ola {
namespace network {

using std::string;
using std::vector;

/*
 * Return a vector of interfaces on the system.
 */
vector<Interface> PosixInterfacePicker::GetInterfaces(
    bool include_loopback) const {
  vector<Interface> interfaces;

#ifdef HAVE_SOCKADDR_DL_STRUCT
  string last_dl_iface_name;
  uint8_t hwlen = 0;
  char *hwaddr = NULL;
  int32_t index = Interface::DEFAULT_INDEX;
  uint16_t type = Interface::ARP_VOID_TYPE;
#endif  // HAVE_SOCKADDR_DL_STRUCT

  // create socket to get iface config
  int sd = socket(PF_INET, SOCK_DGRAM, 0);

  if (sd < 0) {
    OLA_WARN << "Could not create socket " << strerror(errno);
    return interfaces;
  }

  SocketCloser closer(sd);

  // use ioctl to get a listing of interfaces
  char *buffer;  // holds the iface data
  unsigned int lastlen = 0;  // the amount of data returned by the last ioctl
  unsigned int len = INITIAL_IFACE_COUNT;

  while (true) {
    struct ifconf ifc;
    ifc.ifc_len = len * sizeof(struct ifreq);
    buffer = new char[ifc.ifc_len];
    ifc.ifc_buf = buffer;

    if (ioctl(sd, SIOCGIFCONF, &ifc) < 0) {
      if (errno != EINVAL || lastlen != 0) {
        OLA_WARN << "ioctl error " << strerror(errno);
        delete[] buffer;
        return interfaces;
      }
    } else {
      if (static_cast<unsigned int>(ifc.ifc_len) == lastlen) {
        lastlen = ifc.ifc_len;
        break;
      }
      lastlen = ifc.ifc_len;
    }
    len += IFACE_COUNT_INC;
    delete[] buffer;
  }

  // loop through each iface
  for (char *ptr = buffer; ptr < buffer + lastlen;) {
    struct ifreq *iface = (struct ifreq*) ptr;
    ptr += GetIfReqSize(ptr);

#ifdef HAVE_SOCKADDR_DL_STRUCT
    if (iface->ifr_addr.sa_family == AF_LINK) {
      struct sockaddr_dl *sdl = (struct sockaddr_dl*) &iface->ifr_addr;
      last_dl_iface_name.assign(sdl->sdl_data, sdl->sdl_nlen);
      hwaddr = sdl->sdl_data + sdl->sdl_nlen;
      hwlen = sdl->sdl_alen;
      if (sdl->sdl_index != 0) {
        // According to net/if_dl.h
        index = sdl->sdl_index;
      }
      type = sdl->sdl_type;
    }
#endif  // HAVE_SOCKADDR_DL_STRUCT

    // look for AF_INET interfaces only
    if (iface->ifr_addr.sa_family != AF_INET) {
      OLA_DEBUG << "Skipping " << iface->ifr_name
                << " because it's not af_inet";
      continue;
    }

    struct ifreq ifrcopy = *iface;
    if (ioctl(sd, SIOCGIFFLAGS, &ifrcopy) < 0) {
      OLA_WARN << "ioctl error for " << iface->ifr_name << ": "
               << strerror(errno);
      continue;
    }

    if (!(ifrcopy.ifr_flags & IFF_UP)) {
      OLA_DEBUG << "Skipping " << iface->ifr_name
                << " because it's down";
      continue;
    }

    Interface interface;
    interface.name = iface->ifr_name;

    if (ifrcopy.ifr_flags & IFF_LOOPBACK) {
      if (include_loopback) {
        interface.loopback = true;
      } else {
        OLA_DEBUG << "Skipping " << iface->ifr_name
                  << " because it's a loopback";
        continue;
      }
    }

#ifdef HAVE_SOCKADDR_DL_STRUCT
    if (interface.name == last_dl_iface_name) {
      interface.index = index;
      interface.type = type;
      // The only way hwaddr is non-null is if HAVE_SOCKADDR_DL_STRUCT is
      // defined.
      if (hwaddr) {
        if (hwlen == MACAddress::LENGTH) {
          interface.hw_address = MACAddress(reinterpret_cast<uint8_t*>(hwaddr));
        } else {
          OLA_WARN << "hwlen was not expected length, so didn't obtain MAC "
                   << "address; got " << static_cast<int>(hwlen)
                   << ", expecting " << MACAddress::LENGTH;
        }
      }
    }
#endif  // HAVE_SOCKADDR_DL_STRUCT

    struct sockaddr_in *sin = (struct sockaddr_in *) &iface->ifr_addr;
    interface.ip_address = IPV4Address(sin->sin_addr.s_addr);

    // fetch bcast address
#ifdef SIOCGIFBRDADDR
    if (ifrcopy.ifr_flags & IFF_BROADCAST) {
      if (ioctl(sd, SIOCGIFBRDADDR, &ifrcopy) < 0) {
        OLA_WARN << "ioctl error " << strerror(errno);
      } else {
        sin = (struct sockaddr_in *) &ifrcopy.ifr_broadaddr;
        interface.bcast_address = IPV4Address(sin->sin_addr.s_addr);
      }
    }
#endif  // SIOCGIFBRDADDR

    // fetch subnet address
#ifdef  SIOCGIFNETMASK
    if (ioctl(sd, SIOCGIFNETMASK, &ifrcopy) < 0) {
      OLA_WARN << "ioctl error " << strerror(errno);
    } else {
      sin = (struct sockaddr_in *) &ifrcopy.ifr_broadaddr;
      interface.subnet_mask = IPV4Address(sin->sin_addr.s_addr);
    }
#endif  // SIOCGIFNETMASK

    // fetch hardware address
#ifdef SIOCGIFHWADDR
    if (ifrcopy.ifr_flags & SIOCGIFHWADDR) {
      if (ioctl(sd, SIOCGIFHWADDR, &ifrcopy) < 0) {
        OLA_WARN << "ioctl error " << strerror(errno);
      } else {
        interface.type = ifrcopy.ifr_hwaddr.sa_family;
        // TODO(Peter): We probably shouldn't do this if it's not ARPHRD_ETHER
        interface.hw_address = MACAddress(
            reinterpret_cast<uint8_t*>(ifrcopy.ifr_hwaddr.sa_data));
      }
    }
#endif  // SIOCGIFHWADDR

    // fetch index
#ifdef SIOCGIFINDEX
    if (ifrcopy.ifr_flags & SIOCGIFINDEX) {
      if (ioctl(sd, SIOCGIFINDEX, &ifrcopy) < 0) {
        OLA_WARN << "ioctl error " << strerror(errno);
      } else {
#if defined(__FreeBSD__) || defined(__DragonFly__)
        interface.index = ifrcopy.ifr_index;
#else
        interface.index = ifrcopy.ifr_ifindex;
#endif  // defined(__FreeBSD__) || defined(__DragonFly__)
      }
    }
#elif defined(HAVE_IF_NAMETOINDEX)
    // fetch index on NetBSD and other platforms without SIOCGIFINDEX
    unsigned int index = if_nametoindex(iface->ifr_name);
    if (index != 0) {
      interface.index = index;
    }
#endif  // SIOCGIFINDEX

    /* ok, if that all failed we should prob try and use sysctl to work out the
     * broadcast and hardware addresses
     * I'll leave that for another day
     */
    OLA_DEBUG << "Found: " << interface.name << ", "
              << interface.ip_address << ", "
              << interface.hw_address;
    interfaces.push_back(interface);
  }
  delete[] buffer;
  return interfaces;
}


/*
 * Return the size of an ifreq structure in a cross platform manner.
 * @param data a pointer to an ifreq structure
 * @return the size of the ifreq structure
 */
unsigned int PosixInterfacePicker::GetIfReqSize(const char *data) const {
  const struct ifreq *iface = (struct ifreq*) data;
  unsigned int socket_len = SockAddrLen(iface->ifr_addr);

  // We can't assume sizeof(ifreq) = IFNAMSIZ + sizeof(sockaddr), this isn't
  // the case on some 64bit linux systems.
  if (socket_len > sizeof(struct ifreq) - IFNAMSIZ) {
    return IFNAMSIZ + socket_len;
  } else {
    return sizeof(struct ifreq);
  }
}
}  // namespace network
}  // namespace ola
