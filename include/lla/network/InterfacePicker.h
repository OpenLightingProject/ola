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
 * InterfacePicker.h
 * Choose an interface to listen on
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef LLA_INTERFACE_PICKER_H
#define LLA_INTERFACE_PICKER_H

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <netinet/in.h>
#include <string>
#include <vector>

#ifdef HAVE_GETIFADDRS
 #ifdef HAVE_LINUX_IF_PACKET_H
   #define LLA_USE_GETIFADDRS
 #endif
#endif


namespace lla {
namespace network {

enum { MAC_LENGTH = 6 };

/*
 * Represents an interface.
 */
class Interface {
  public:
    std::string name;
    struct in_addr ip_address;
    struct in_addr bcast_address;
    int8_t hw_address[MAC_LENGTH];
};


/*
 * Chooses an interface
 */
class InterfacePicker {
  public:
    InterfacePicker() {}
    ~InterfacePicker() {}
    bool ChooseInterface(Interface &interface,
                         const std::string &preferred_ip) const;

    std::vector<Interface> GetInterfaces() const;
};

} //network
} //lla
#endif

