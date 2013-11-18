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
 * ResponderNetworkController.h
 * Talks to the machine's network systems to get/set data.
 * Copyright (C) 2013 Peter Newman
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file ResponderNetworkController.h
 * @brief Gets/sets config about a network.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_RESPONDERNETWORKCONTROLLER_H_
#define INCLUDE_OLA_RDM_RESPONDERNETWORKCONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "ola/rdm/RDMEnums.h"
#include "ola/network/Interface.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/IPV4Address.h"

namespace ola {
namespace rdm {

/**
 * @brief Gets global network information.
 */
class GlobalNetworkGetter {
  public:
    GlobalNetworkGetter() {}
    virtual ~GlobalNetworkGetter() {}

    /**
     * Get the interface picker
     */
    virtual const ola::network::InterfacePicker *GetInterfacePicker() const {
      return m_interface_picker.get();
    };

    /**
     * Get the DHCP status of an interface
     * @param iface the interface to check the DHCP status of
     * @return true if the interface is using DHCP, false otherwise
     */
    virtual bool GetDHCPStatus(const ola::network::Interface &iface) const = 0;

    /**
     * Get the IPv4 default route
     * @return the machine's default route as an IPV4Address object
     */
    virtual const ola::network::IPV4Address GetIPV4DefaultRoute() const = 0;

    /**
     * Get the hostname
     */
    virtual const std::string GetHostname() const = 0;

    /**
     * Get the domain name
     */
    virtual const std::string GetDomainName() const = 0;

    /**
     * Get name servers
     */
    virtual bool GetNameServers(
        std::vector<ola::network::IPV4Address> *name_servers) const = 0;

  protected:
    std::auto_ptr<ola::network::InterfacePicker> m_interface_picker;
};
// TODO(Peter): Set global network information.
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RESPONDERNETWORKCONTROLLER_H_
