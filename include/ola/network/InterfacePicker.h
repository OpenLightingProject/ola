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

#ifndef INCLUDE_OLA_NETWORK_INTERFACEPICKER_H_
#define INCLUDE_OLA_NETWORK_INTERFACEPICKER_H_

#include <netinet/in.h>
#include <string>
#include <vector>

namespace ola {
namespace network {

enum { MAC_LENGTH = 6 };

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
    int8_t hw_address[MAC_LENGTH];
};


/*
 * Chooses an interface
 */
class InterfacePicker {
  public:
    InterfacePicker() {}
    virtual ~InterfacePicker() {}
    bool ChooseInterface(Interface *interface,
                         const std::string &preferred_ip) const;

    virtual std::vector<Interface> GetInterfaces() const;
  private:
    static const unsigned int INITIAL_IFACE_COUNT = 10;
    static const unsigned int IFACE_COUNT_INC = 5;

    unsigned int GetIfReqSize(const char *data) const;
};
}  // network
}  // ola
#endif  // INCLUDE_OLA_NETWORK_INTERFACEPICKER_H_

