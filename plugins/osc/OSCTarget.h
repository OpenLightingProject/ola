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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
  string osc_address;

  // The default constructor.
  OSCTarget() {}

  // The copy constructor
  OSCTarget(const OSCTarget &target)
      : socket_address(target.socket_address),
        osc_address(target.osc_address) {
  }

  // A constuctor that initializes the member variables as well.
  OSCTarget(const ola::network::IPV4SocketAddress &socket_address,
            const std::string &osc_address)
      : socket_address(socket_address),
        osc_address(osc_address) {
  }

  std::string ToString() const {
    return socket_address.ToString() + osc_address;
  }

  /**
   * @brief A helper function to write a OSCTarget to an ostream.
   * @param out the ostream
   * @param uid the UID to write.
   */
  friend ostream& operator<<(ostream &out, const OSCTarget &target) {
    return out << target.ToString();
  }
};
}  // namespace osc
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OSC_OSCTARGET_H_
