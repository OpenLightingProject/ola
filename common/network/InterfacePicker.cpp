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


Interface::Interface() {
  ip_address.s_addr = 0;
  bcast_address.s_addr = 0;
  subnet_address.s_addr = 0;
  memset(hw_address, 0, MAC_LENGTH);
}


Interface::Interface(const Interface &other) {
  name = other.name;
  ip_address = other.ip_address;
  bcast_address = other.bcast_address;
  subnet_address = other.subnet_address;
  memcpy(hw_address, other.hw_address, MAC_LENGTH);
}


Interface& Interface::operator=(const Interface &other) {
  if (this != &other) {
    name = other.name;
    ip_address = other.ip_address;
    bcast_address = other.bcast_address;
    subnet_address = other.subnet_address;
    memcpy(hw_address, other.hw_address, MAC_LENGTH);
  }
  return *this;
}


bool Interface::operator==(const Interface &other) {
  return (name == other.name &&
          ip_address.s_addr == other.ip_address.s_addr &&
          subnet_address.s_addr == other.subnet_address.s_addr);
}


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
  struct in_addr wanted_ip;
  vector<Interface> interfaces = GetInterfaces();

  if (!interfaces.size()) {
    OLA_INFO << "No interfaces found";
    return false;
  }

  vector<Interface>::const_iterator iter;
  if (!ip_or_name.empty()) {
    if (StringToAddress(ip_or_name, wanted_ip)) {
      // search by IP
      for (iter = interfaces.begin(); iter != interfaces.end(); ++iter) {
        if ((*iter).ip_address.s_addr == wanted_ip.s_addr) {
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
