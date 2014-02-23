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
 * NetworkManager.cpp
 * Copyright (C) 2013-2014 Peter Newman
 */

#include "common/rdm/NetworkManager.h"

#include <string>
#include <vector>

#include "ola/network/NetworkUtils.h"

namespace ola {
namespace rdm {

using ola::network::Interface;
using ola::network::InterfacePicker;
using ola::network::IPV4Address;
using std::string;
using std::vector;

const InterfacePicker *NetworkManager::GetInterfacePicker() const {
  return m_interface_picker.get();
}

NetworkManagerInterface::DhcpStatus NetworkManager::GetDHCPStatus(
    const Interface&) const {
  // It's a challenge to determine DHCP state, so we already return
  // DHCP_STATUS_UNKNOWN.
  return DHCP_STATUS_UNKNOWN;
}

bool NetworkManager::GetIPV4DefaultRoute(
    IPV4Address *default_route) const {
  // if_index is ignored for now.
  int32_t if_index;
  return ola::network::DefaultRoute(&if_index, default_route);
}

const string NetworkManager::GetHostname() const {
  return ola::network::Hostname();
}

const string NetworkManager::GetDomainName() const {
  return ola::network::DomainName();
}

bool NetworkManager::GetNameServers(vector<IPV4Address> *name_servers) const {
  return ola::network::NameServers(name_servers);
}
}  // namespace rdm
}  // namespace ola
