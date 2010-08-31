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
 * Interface.h
 * Represents a network interface.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_NETWORK_INTERFACE_H_
#define INCLUDE_OLA_NETWORK_INTERFACE_H_

#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include <string>
#include <vector>

namespace ola {
namespace network {

enum { MAC_LENGTH = 6 };
enum { IPV4_LENGTH = 4 };

/*
 * Represents an interface.
 */
class Interface {
  public:
    Interface();
    Interface(const Interface &other);
    Interface& operator=(const Interface &other);
    bool operator==(const Interface &other);

    std::string name;
    struct in_addr ip_address;
    struct in_addr bcast_address;
    struct in_addr subnet_address;
    int8_t hw_address[MAC_LENGTH];
};
}  // network
}  // ola
#endif  // INCLUDE_OLA_NETWORK_INTERFACE_H_

