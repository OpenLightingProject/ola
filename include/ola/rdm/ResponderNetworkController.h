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
 * Talks to the machine's network to get/set data.
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

#include <string>
#include <vector>

#include "ola/rdm/RDMEnums.h"
#include "ola/network/IPV4Address.h"

namespace ola {
namespace rdm {

using std::string;
using std::vector;

typedef vector<ola::network::IPV4Address> NameServers;

/**
 * @brief Gets DNS information.
 */
class DNSGetter {
  public:
    DNSGetter() {}
    virtual ~DNSGetter() {}

    virtual string GetHostname() = 0;
    virtual string GetDomainName() = 0;
    virtual NameServers GetNameServers() = 0;
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RESPONDERNETWORKCONTROLLER_H_
