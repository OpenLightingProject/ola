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
 * FakeNetworkManager.h
 * Talks to the machine's network systems to get/set data.
 * Copyright (C) 2013 Peter Newman
 */

#ifndef COMMON_RDM_FAKENETWORKMANAGER_H_
#define COMMON_RDM_FAKENETWORKMANAGER_H_


#include <string>
#include <vector>

#include "common/network/FakeInterfacePicker.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/Interface.h"
#include "ola/network/InterfacePicker.h"
#include "ola/rdm/NetworkManagerInterface.h"

namespace ola {
namespace rdm {

/**
 * @brief An implementation of NetworkManagerInterface which simulates a
 * network configuration.
 */
class FakeNetworkManager : public NetworkManagerInterface {
 public:
  /**
   * @brief Create a new FakeNetworkManager
   * @param interfaces the interfaces to return
   * @param ipv4_default_route_if_index the interface that has the default
       gateway
   * @param ipv4_default_route the default gateway
   * @param hostname the hostname
   * @param domain_name the domain name
   * @param name_servers the name servers to return.
   */
  FakeNetworkManager(
      const std::vector<ola::network::Interface> &interfaces,
      const int32_t ipv4_default_route_if_index,
      const ola::network::IPV4Address ipv4_default_route,
      const std::string &hostname,
      const std::string &domain_name,
      const std::vector<ola::network::IPV4Address> &name_servers);
  ~FakeNetworkManager() {}

  const ola::network::InterfacePicker *GetInterfacePicker() const;

  rdm_dhcp_status GetDHCPStatus(const ola::network::Interface &iface) const;

  bool GetIPV4DefaultRoute(
      int32_t *if_index,
      ola::network::IPV4Address *default_route) const;

  const std::string GetHostname() const;

  const std::string GetDomainName() const;

  bool GetNameServers(
      std::vector<ola::network::IPV4Address> *name_servers) const;

 private:
  ola::network::FakeInterfacePicker m_interface_picker;
  int32_t m_ipv4_default_route_if_index;
  ola::network::IPV4Address m_ipv4_default_route;
  std::string m_hostname;
  std::string m_domain_name;
  std::vector<ola::network::IPV4Address> m_name_servers;
};
}  // namespace rdm
}  // namespace ola
#endif  // COMMON_RDM_FAKENETWORKMANAGER_H_
