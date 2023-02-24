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
 * NetworkManager.h
 * Talks to the machine's network systems to get/set data.
 * Copyright (C) 2013 Peter Newman
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file NetworkManager.h
 * @brief Gets/sets real config about a network.
 * @}
 */

#ifndef COMMON_RDM_NETWORKMANAGER_H_
#define COMMON_RDM_NETWORKMANAGER_H_

#include <memory>
#include <string>
#include <vector>

#include "ola/rdm/NetworkManagerInterface.h"

namespace ola {
namespace rdm {

/**
 * @brief A NetworkManager which reflects the actual host network
 * configuration.
 */
class NetworkManager : public NetworkManagerInterface {
 public:
  NetworkManager() : NetworkManagerInterface() {
    m_interface_picker.reset(ola::network::InterfacePicker::NewPicker());
  }

  const ola::network::InterfacePicker *GetInterfacePicker() const;
  rdm_dhcp_status GetDHCPStatus(const ola::network::Interface &iface) const;
  bool GetIPV4DefaultRoute(int32_t *if_index,
                           ola::network::IPV4Address *default_route) const;
  const std::string GetHostname() const;
  const std::string GetDomainName() const;
  bool GetNameServers(
      std::vector<ola::network::IPV4Address> *name_servers) const;

 private:
  std::auto_ptr<ola::network::InterfacePicker> m_interface_picker;
};
}  // namespace rdm
}  // namespace ola
#endif  // COMMON_RDM_NETWORKMANAGER_H_
