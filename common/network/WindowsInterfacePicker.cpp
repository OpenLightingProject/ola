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
 * WindowsInterfacePicker.cpp
 * Chooses an interface to listen on
 * Copyright (C) 2005-2010 Simon Newton
 */

typedef int socklen_t;
#include <winsock2.h>
#include <Lm.h>
#include <iphlpapi.h>
#include <unistd.h>
#include <vector>

#include "ola/Logging.h"
#include "ola/network/InterfacePicker.h"

namespace ola {
namespace network {

using std::vector;

/*
 * Return a vector of interfaces on the system.
 */
vector<Interface> WindowsInterfacePicker::GetInterfaces() const {
  vector<Interface> interfaces;

  PIP_ADAPTER_INFO pAdapter = NULL;
  PIP_ADAPTER_INFO pAdapterInfo;
  IP_ADDR_STRING *ipAddress;
  ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
  unsigned long net, mask;

  while (1) {
    pAdapterInfo = (IP_ADAPTER_INFO*) malloc(ulOutBufLen);
    if (!pAdapterInfo) {
      OLA_WARN << "Error allocating memory needed for GetAdaptersinfo";
      return interfaces;
    }

    // if ulOutBufLen isn't long enough it'll be set to the size of the buffer
    // required
    DWORD status = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
    if (status == NO_ERROR)
      break;

    free(pAdapterInfo);
    if (status != ERROR_BUFFER_OVERFLOW) {
      OLA_WARN << "GetAdaptersInfo failed with error: " <<
        static_cast<int>(status);
      return interfaces;
    }
  }

  for (pAdapter = pAdapterInfo;
       pAdapter && pAdapter < pAdapterInfo + ulOutBufLen;
       pAdapter = pAdapter->Next) {
    if (pAdapter->Type != MIB_IF_TYPE_ETHERNET)
      continue;

    for (ipAddress = &pAdapter->IpAddressList; ipAddress;
         ipAddress = ipAddress->Next) {
      net = inet_addr(ipAddress->IpAddress.String);
      // Windows doesn't seem to have the notion of an interface being 'up'
      // so we check if this interface has an address assigned.
      if (net) {
        Interface interface;
        interface.name = pAdapter->AdapterName;  // IFNAME_SIZE
        memcpy(interface.hw_address, pAdapter->Address, MAC_LENGTH);
        struct sockaddr_in *sin = (struct sockaddr_in *) &iface->ifr_addr;
        interface.ip_address = sin->sin_addr;

        mask = inet_addr(ipAddress->IpMask.String);
        interface.bcast_addr = ((interface.ip_address & mask) |
                                (0xFFFFFFFF ^ mask));

        interfaces.push_back(interface);
      }
    }
  }
  free(pAdapterInfo);
  return interfaces;
}
}  // network
}  // ola
