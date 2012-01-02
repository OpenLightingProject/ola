/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * InterfacePicker.cpp
 * Chooses an interface to listen on
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string.h>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/NetworkUtils.h"

#ifdef WIN32
#include "common/network/WindowsInterfacePicker.h"
#else
#include "common/network/PosixInterfacePicker.h"
#endif

namespace ola {
namespace network {

using std::string;
using std::vector;


/*
 * Select an interface to use
 * @param interface, the interface to populate
 * @param ip_or_name the ip address or interface name  of the local interface
 *   we'd prefer to use.
 * @return true if we found an interface, false otherwise
 */
bool InterfacePicker::ChooseInterface(Interface *iface,
                                      const string &ip_or_name) const {
  bool found = false;
  vector<Interface> interfaces = GetInterfaces();

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

  if (!found)
    *iface = interfaces[0];
  OLA_DEBUG << "Using interface " << iface->name << " (" <<
    iface->ip_address << ")";
  return true;
}


/*
 * Create the appropriate picker
 */
InterfacePicker *InterfacePicker::NewPicker() {
#ifdef WIN32
  return new WindowsInterfacePicker();
#else
  return new PosixInterfacePicker();
#endif
}
}  // network
}  // ola
