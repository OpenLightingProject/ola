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
 * FakeInterfacePicker.h
 * A fake interface picker
 * Copyright (C) 2005-2009 Simon Newton
 */
#ifndef COMMON_NETWORK_FAKEINTERFACEPICKER_H_
#define COMMON_NETWORK_FAKEINTERFACEPICKER_H_

#include <vector>
#include "ola/Logging.h"
#include "ola/network/InterfacePicker.h"

namespace ola {
namespace network {


using std::vector;


/*
 * The InterfacePicker for dummy systems and testing
 */
class FakeInterfacePicker: public InterfacePicker {
  public:
    explicit FakeInterfacePicker(const vector<Interface> &interfaces)
        : InterfacePicker(),
          m_interfaces(interfaces) {
      OLA_INFO << "Started with " << interfaces.size() << " fake interfaces";
      OLA_INFO << "Transfered to " << m_interfaces.size() << " fake interfaces";
    }

    std::vector<Interface> GetInterfaces(bool loopback) const {
      OLA_INFO << "Getting fake interfaces";
      OLA_INFO << "Found " << m_interfaces.size() << " fake interfaces";
      return m_interfaces;
      (void) loopback;
    }
  private:
    const vector<Interface> &m_interfaces;
};
}  // namespace network
}  // namespace ola
#endif  // COMMON_NETWORK_FAKEINTERFACEPICKER_H_
