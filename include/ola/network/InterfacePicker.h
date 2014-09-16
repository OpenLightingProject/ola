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
 * InterfacePicker.h
 * Choose an interface to listen on
 * Copyright (C) 2005 Simon Newton
 */

#ifndef INCLUDE_OLA_NETWORK_INTERFACEPICKER_H_
#define INCLUDE_OLA_NETWORK_INTERFACEPICKER_H_

#include <ola/network/Interface.h>
#include <string>
#include <vector>

namespace ola {
namespace network {


/**
 * @addtogroup network
 * @{
 */

/**
 * @brief Given some intial parameters, find the best network interface to use.
 *
 * The InterfacePicker tries to find a valid network interface that matches:
 *  - an interface name i.e. eth0
 *  - an IP address
 *  - an index.
 *
 * If the requested interface can't be found, it can fall back to returning any
 * configured interface.
 */
class InterfacePicker {
 public:
  struct Options {
   public:
    /**
     * True to include the loopback interface(s) when searching
     */
    bool include_loopback;

    /**
     * True if we're only interested in the specific interface when
     * searching, false to ensure we return something even if we didn't find a match
     */
    bool specific_only;

    Options()
      : include_loopback(false),
        specific_only(false) {
    }
  };

  /**
   * @brief Constructor
   */
  InterfacePicker() {}

  /**
   * @brief Destructor
   */
  virtual ~InterfacePicker() {}

  // stupid windows, 'interface' seems to be a struct so we use iface here.
  bool ChooseInterface(
      Interface *iface,
      const std::string &ip_or_name,
      const Options &options = Options()) const;
  bool ChooseInterface(
      Interface *iface,
      int32_t index,
      const Options &options = Options()) const;

  virtual std::vector<Interface> GetInterfaces(bool include_loopback) const = 0;

  static InterfacePicker *NewPicker();
};
/**
 * @}
 */
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_INTERFACEPICKER_H_

