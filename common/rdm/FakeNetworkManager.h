/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * FakeNetworkManager.h
 * Talks to the machine's network systems to get/set data.
 * Copyright (C) 2013-2014 Peter Newman
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
 * @brief An implementation of NetworkManagerInterface which simulated a
 * network configuration.
 */
class FakeNetworkManager : public NetworkManagerInterface {
 public:
  /**
   * @brief Create a new FakeNetworkManager
   * @param interfaces the interfaces to return
   * @param ipv4_default_route the default gateway
   * @param hostname the hostname
   * @param domain_name the domain name
   * @param name_servers the name servers to return.
   */
  FakeNetworkManager(
      const std::vector<ola::network::Interface> &interfaces,
      const ola::network::IPV4Address ipv4_default_route,
      const std::string &hostname,
      const std::string &domain_name,
      const std::vector<ola::network::IPV4Address> &name_servers);
  ~FakeNetworkManager() {}

  /**
   * Get the interface picker
   */
  const ola::network::InterfacePicker *GetInterfacePicker() const;

  /**
   * Get the DHCP status of an interface
   * @param iface the interface to check the DHCP status of
   * @return One of DHCP_STATUS_ENABLED, DHCP_STATUS_DISABLED or
   * DHCP_STATUS_UNKNOWN.
   */
  rdm_dhcp_status GetDHCPStatus(const ola::network::Interface &iface) const;

  /**
   * Get the IPv4 default route
   * @param[out] the machine's default route as an IPV4Address object
   * @return true if we managed to fetch the default route, false otherwise
   * @note if it manages to fetch the route information and there isn't a route,
   * it will return the special wildcard address, which can be tested for with
   * IsWildcard().
   */
  bool GetIPV4DefaultRoute(
      ola::network::IPV4Address *default_route) const;

  /**
   * Get the hostname
   */
  const std::string GetHostname() const;

  /**
   * Get the domain name
   */
  const std::string GetDomainName() const;

  /**
   * Get name servers
   */
  bool GetNameServers(
      std::vector<ola::network::IPV4Address> *name_servers) const;

 private:
  ola::network::FakeInterfacePicker m_interface_picker;
  ola::network::IPV4Address m_ipv4_default_route;
  std::string m_hostname;
  std::string m_domain_name;
  std::vector<ola::network::IPV4Address> m_name_servers;
};
}  // namespace rdm
}  // namespace ola
#endif  // COMMON_RDM_FAKENETWORKMANAGER_H_
