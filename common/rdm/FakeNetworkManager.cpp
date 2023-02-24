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
 * FakeNetworkManager.cpp
 * A NetworkManager which uses a simulated network config.
 * Copyright (C) 2014 Simon Newton
 */

#include "common/rdm/FakeNetworkManager.h"

#include <string>
#include <vector>


namespace ola {
namespace rdm {

using ola::network::Interface;
using ola::network::InterfacePicker;
using ola::network::IPV4Address;
using std::string;
using std::vector;

FakeNetworkManager::FakeNetworkManager(
    const vector<Interface> &interfaces,
    const int32_t ipv4_default_route_if_index,
    const IPV4Address ipv4_default_route,
    const string &hostname,
    const string &domain_name,
    const vector<IPV4Address> &name_servers)
    : NetworkManagerInterface(),
      m_interface_picker(interfaces),
      m_ipv4_default_route_if_index(ipv4_default_route_if_index),
      m_ipv4_default_route(ipv4_default_route),
      m_hostname(hostname),
      m_domain_name(domain_name),
      m_name_servers(name_servers) {
}

const InterfacePicker *FakeNetworkManager::GetInterfacePicker() const {
  return &m_interface_picker;
}

rdm_dhcp_status FakeNetworkManager::GetDHCPStatus(
    const Interface &iface) const {
  // Mix things up a bit. The status depends on the index.
  return static_cast<rdm_dhcp_status>(iface.index % DHCP_STATUS_MAX);
}

bool FakeNetworkManager::GetIPV4DefaultRoute(
    int32_t *if_index,
    IPV4Address *default_route) const {
  *if_index = m_ipv4_default_route_if_index;
  *default_route = m_ipv4_default_route;
  return true;
}

const string FakeNetworkManager::GetHostname() const {
  return m_hostname;
}

const string FakeNetworkManager::GetDomainName() const {
  return m_domain_name;
}

bool FakeNetworkManager::GetNameServers(
    vector<IPV4Address> *name_servers) const {
  *name_servers = m_name_servers;
  return true;
}
}  // namespace rdm
}  // namespace ola
