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
 * RealGlobalNetworkGetter.cpp
 * Copyright (C) 2013 Peter Newman
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RealGlobalNetworkGetter.h"

namespace ola {
namespace rdm {

/**
 * Get the interface picker
 */
const InterfacePicker *RealGlobalNetworkGetter::GetInterfacePicker() const {
  OLA_INFO << "Getting picker";
  return InterfacePicker::NewPicker();
}


/**
 * Get the DHCP status of an interface
 * @param iface the interface to check the DHCP status of
 * @return true if using DHCP, false otherwise
 */
bool RealGlobalNetworkGetter::GetDHCPStatus(
    const ola::network::Interface &iface) const {
  // TODO(Peter): Fixme - actually do the work!
  if (iface.index > 0) {}
  return false;
}


/**
 * Get the IPv4 default route
 */
IPV4Address RealGlobalNetworkGetter::GetIPV4DefaultRoute() {
  return IPV4Address();
}


/**
 * Get the hostname
 */
string RealGlobalNetworkGetter::GetHostname() {
  return ola::network::Hostname();
}


/**
 * Get the domain name
 */
string RealGlobalNetworkGetter::GetDomainName() {
  return ola::network::Domain();
}

/**
 * Get name servers
 */
NameServers RealGlobalNetworkGetter::GetNameServers() {
  return ola::network::Nameservers();
}
}  // namespace rdm
}  // namespace ola
