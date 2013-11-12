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
 * InterfacePicker.h
 * Choose an interface to listen on
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef INCLUDE_OLA_NETWORK_INTERFACEPICKER_H_
#define INCLUDE_OLA_NETWORK_INTERFACEPICKER_H_

#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include <ola/network/Interface.h>
#include <string>
#include <vector>

namespace ola {
namespace network {


/*
 * Chooses an interface
 */
class InterfacePicker {
  public:
    struct ChooseInterfaceOptions {
      public:
        // Include the loopback interface when searching
        bool include_loopback;
        /**
         * True if we're only interested in the specific interface when
         * searching, false to ensure we return something even if we didn't find a match
         */
        bool specific_only;

        ChooseInterfaceOptions()
          : include_loopback(false),
            specific_only(false) {
        }
    };

    InterfacePicker() {}
    virtual ~InterfacePicker() {}

    // stupid windows, 'interface' seems to be a struct so we use iface here.
    bool ChooseInterface(
        Interface *iface,
        const std::string &ip_or_name,
        const ChooseInterfaceOptions &options = ChooseInterfaceOptions()) const;
    bool ChooseInterface(
        Interface *iface,
        int32_t index,
        const ChooseInterfaceOptions &options = ChooseInterfaceOptions()) const;

    virtual std::vector<Interface> GetInterfaces(
        bool include_loopback) const = 0;

    static InterfacePicker *NewPicker();
};
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_INTERFACEPICKER_H_

