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
 * InterfacePicker.cpp
 * Chooses an interface to listen on
 * Copyright (C) 2005 Simon Newton
 */

#include <string.h>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/NetworkUtils.h"

#ifdef _WIN32
#include "common/network/WindowsInterfacePicker.h"
#else
#include "common/network/PosixInterfacePicker.h"
#endif  // _WIN32

namespace ola {
namespace network {

using std::string;
using std::vector;


/*
 * Select an interface to use
 * @param iface, the interface to populate
 * @param ip_or_name the IP address or interface name of the local interface
 *   we'd prefer to use.
 * @param options an Options struct configuring ChooseInterface
 * @return true if we found an interface, false otherwise
 */
// TODO(Simon): Change these to callback based code to reduce duplication.
bool InterfacePicker::ChooseInterface(
    Interface *iface,
    const string &ip_or_name,
    const Options &options) const {
  bool found = false;
  vector<Interface> interfaces = GetInterfaces(options.include_loopback);

  if (interfaces.empty()) {
    OLA_INFO << "No interfaces found";
    return false;
  }

  vector<Interface>::const_iterator iter;
  if (!ip_or_name.empty()) {
    IPV4Address wanted_ip;
    if (IPV4Address::FromString(ip_or_name, &wanted_ip)) {
      // search by IP
      for (iter = interfaces.begin(); iter != interfaces.end(); ++iter) {
        if (iter->ip_address == wanted_ip) {
          *iface = *iter;
          found = true;
          break;
        }
      }
    } else {
      // search by interface name
      for (iter = interfaces.begin(); iter != interfaces.end(); ++iter) {
        if (iter->name == ip_or_name) {
          *iface = *iter;
          found = true;
          break;
        }
      }
    }
  }

  if (!found && options.specific_only) {
    return false;  // No match and being fussy
  }

  if (!found) {
    *iface = interfaces[0];
  }
  OLA_DEBUG << "Using interface " << iface->name << " ("
            << iface->ip_address << ")";
  return true;
}


/*
 * Select an interface to use by index
 * @param iface, the interface to populate
 * @param index the index of the local interface we'd prefer to use.
 * @param options an Options struct configuring ChooseInterface
 * @return true if we found an interface, false otherwise
 */
// TODO(Simon): Change these to callback based code to reduce duplication.
bool InterfacePicker::ChooseInterface(
    Interface *iface,
    int32_t index,
    const Options &options) const {
  bool found = false;
  vector<Interface> interfaces = GetInterfaces(options.include_loopback);

  if (interfaces.empty()) {
    OLA_INFO << "No interfaces found";
    return false;
  }

  vector<Interface>::const_iterator iter;
  // search by index
  for (iter = interfaces.begin(); iter != interfaces.end(); ++iter) {
    if (iter->index == index) {
      *iface = *iter;
      found = true;
      break;
    }
  }

  if (!found && options.specific_only) {
    return false;  // No match and being fussy
  }

  if (!found) {
    *iface = interfaces[0];
  }
  OLA_DEBUG << "Using interface " << iface->name << " ("
            << iface->ip_address << ") with index " << iface->index;
  return true;
}


/*
 * Create the appropriate picker
 */
InterfacePicker *InterfacePicker::NewPicker() {
#ifdef _WIN32
  return new WindowsInterfacePicker();
#else
  return new PosixInterfacePicker();
#endif  // _WIN32
}
}  // namespace network
}  // namespace ola
