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
 * WindowsInterfacePicker.h
 * Choose an interface to listen on
 * Copyright (C) 2005 Simon Newton
 */

#ifndef COMMON_NETWORK_WINDOWSINTERFACEPICKER_H_
#define COMMON_NETWORK_WINDOWSINTERFACEPICKER_H_

#include <vector>
#include "ola/network/InterfacePicker.h"

namespace ola {
namespace network {

/*
 * The InterfacePicker for windows
 */
class WindowsInterfacePicker: public InterfacePicker {
 public:
    std::vector<Interface> GetInterfaces(bool include_loopback) const;
};
}  // namespace network
}  // namespace ola
#endif  // COMMON_NETWORK_WINDOWSINTERFACEPICKER_H_

