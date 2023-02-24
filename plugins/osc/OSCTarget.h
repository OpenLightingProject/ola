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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * OSCTarget.h
 * Represents a IP:Port & OSC Address pair used to direct OSC messages.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef PLUGINS_OSC_OSCTARGET_H_
#define PLUGINS_OSC_OSCTARGET_H_

#include <string>
#include "ola/network/SocketAddress.h"

namespace ola {
namespace plugin {
namespace osc {

struct OSCTarget {
  ola::network::IPV4SocketAddress socket_address;
  std::string osc_address;

  // The default constructor.
  OSCTarget() {}

  // The copy constructor
  OSCTarget(const OSCTarget &target)
      : socket_address(target.socket_address),
        osc_address(target.osc_address) {
  }

  // A constructor that initializes the member variables as well.
  OSCTarget(const ola::network::IPV4SocketAddress &socket_address,
            const std::string &osc_address)
      : socket_address(socket_address),
        osc_address(osc_address) {
  }

  void operator=(const OSCTarget &other) {
      socket_address = other.socket_address;
      osc_address = other.osc_address;
  }

  std::string ToString() const {
    return socket_address.ToString() + osc_address;
  }

  /**
   * @brief A helper function to write a OSCTarget to an ostream.
   * @param out the ostream
   * @param target the OSCTarget to write.
   */
  friend std::ostream& operator<<(std::ostream &out, const OSCTarget &target) {
    return out << target.ToString();
  }
};
}  // namespace osc
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OSC_OSCTARGET_H_
