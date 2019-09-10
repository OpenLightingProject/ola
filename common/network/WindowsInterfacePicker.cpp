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
 * WindowsInterfacePicker.cpp
 * Chooses an interface to listen on
 * Copyright (C) 2005 Simon Newton
 */

typedef int socklen_t;
#include <ola/win/CleanWinSock2.h>
#include <Lm.h>
#include <iphlpapi.h>
#include <unistd.h>
#include <vector>

#include "ola/network/IPV4Address.h"
#include "common/network/WindowsInterfacePicker.h"
#include "ola/Logging.h"

namespace ola {
namespace network {

using std::vector;

/*
 * Return a vector of interfaces on the system.
 */
vector<Interface> WindowsInterfacePicker::GetInterfaces(
    bool include_loopback) const {
  vector<Interface> interfaces;

  PIP_ADAPTER_INFO pAdapter = NULL;
  PIP_ADAPTER_INFO pAdapterInfo;
  IP_ADDR_STRING *ipAddress;
  ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
  uint32_t net, mask;

  if (include_loopback) {
    OLA_WARN << "Loopback interface inclusion requested. Loopback might not "
             << "exist on Windows";
  }

  while (1) {
    pAdapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(new uint8_t[ulOutBufLen]);
    if (!pAdapterInfo) {
      OLA_WARN << "Error allocating memory needed for GetAdaptersInfo";
      return interfaces;
    }

    // if ulOutBufLen isn't long enough it will be set to the size of the
    // buffer required
    DWORD status = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
    if (status == NO_ERROR) {
      break;
    }

    delete[] pAdapterInfo;
    if (status != ERROR_BUFFER_OVERFLOW) {
      OLA_WARN << "GetAdaptersInfo failed with error: "
               << static_cast<int>(status);
      return interfaces;
    }
  }

  for (pAdapter = pAdapterInfo;
       pAdapter && pAdapter < pAdapterInfo + ulOutBufLen;
       pAdapter = pAdapter->Next) {
    // Since Vista, wireless interfaces return a different type
    // See https://msdn.microsoft.com/en-us/library/windows/desktop/aa366062(v=vs.85).aspx
    if ((pAdapter->Type != MIB_IF_TYPE_ETHERNET) &&
        (pAdapter->Type != IF_TYPE_IEEE80211)) {
      OLA_INFO << "Skipping " << pAdapter->AdapterName
               << " (" << pAdapter->Description << ")"
               << " as it's not MIB_IF_TYPE_ETHERNET"
               << " or IF_TYPE_IEEE80211, got "
               << pAdapter->Type << " instead";
      continue;
    }

    for (ipAddress = &pAdapter->IpAddressList; ipAddress;
         ipAddress = ipAddress->Next) {
      net = inet_addr(ipAddress->IpAddress.String);
      // Windows doesn't seem to have the notion of an interface being 'up'
      // so we check if this interface has an address assigned.
      if (net) {
        Interface iface;
        iface.name = pAdapter->AdapterName;  // IFNAME_SIZE
        iface.index = pAdapter->Index;
        uint8_t macaddr[MACAddress::LENGTH];
        memcpy(macaddr, pAdapter->Address, MACAddress::LENGTH);
        iface.hw_address = MACAddress(macaddr);
        iface.ip_address = IPV4Address(net);

        mask = inet_addr(ipAddress->IpMask.String);
        iface.subnet_mask = IPV4Address(mask);
        iface.bcast_address = IPV4Address((iface.ip_address.AsInt() & mask) |
                                          (0xFFFFFFFF ^ mask));

        interfaces.push_back(iface);
      }
    }
  }
  delete[] pAdapterInfo;
  return interfaces;
}
}  // namespace network
}  // namespace ola
