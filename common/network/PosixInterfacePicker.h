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
 * PosixInterfacePicker.h
 * Choose an interface to listen on
 * Copyright (C) 2005-2010 Simon Newton
 */
#ifndef COMMON_NETWORK_POSIXINTERFACEPICKER_H_
#define COMMON_NETWORK_POSIXINTERFACEPICKER_H_

#include <vector>
#include "ola/network/InterfacePicker.h"

namespace ola {
namespace network {


/*
 * The InterfacePicker for posix systems
 */
class PosixInterfacePicker: public InterfacePicker {
  public:
    std::vector<Interface> GetInterfaces(bool include_loopback) const;

  private:
    static const unsigned int INITIAL_IFACE_COUNT = 10;
    static const unsigned int IFACE_COUNT_INC = 5;
    unsigned int GetIfReqSize(const char *data) const;
};
}  // namespace network
}  // namespace ola
#endif  // COMMON_NETWORK_POSIXINTERFACEPICKER_H_
