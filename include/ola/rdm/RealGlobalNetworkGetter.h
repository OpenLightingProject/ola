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
 * RealGlobalNetworkGetter.h
 * Talks to the machine's network systems to get/set data.
 * Copyright (C) 2013 Peter Newman
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file RealGlobalNetworkGetter.h
 * @brief Gets/sets real config about a network.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_REALGLOBALNETWORKGETTER_H_
#define INCLUDE_OLA_RDM_REALGLOBALNETWORKGETTER_H_

#include <string>
#include <vector>

#include "ola/rdm/ResponderNetworkController.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/IPV4Address.h"

namespace ola {
namespace rdm {

/**
 * A class which represents a real network getter.
 */
class RealGlobalNetworkGetter: public GlobalNetworkGetter {
  public:
    RealGlobalNetworkGetter()
        : GlobalNetworkGetter() {
    }

  protected:
    const ola::network::InterfacePicker *GetInterfacePicker() const;
    bool GetDHCPStatus(const ola::network::Interface &iface) const;
    ola::network::IPV4Address GetIPV4DefaultRoute();
    std::string GetHostname();
    std::string GetDomainName();
    bool GetNameServers(std::vector<ola::network::IPV4Address> *name_servers);
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_REALGLOBALNETWORKGETTER_H_
