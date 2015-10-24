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
 * MACAddress.h
 * Represents a MAC Address
 * Copyright (C) 2013 Peter Newman
 */

#ifndef INCLUDE_OLA_NETWORK_MACADDRESS_H_
#define INCLUDE_OLA_NETWORK_MACADDRESS_H_

/**
 * @addtogroup network
 * @{
 * @file MACAddress.h
 * @brief Represents a MAC Address.
 * @}
 */

#include <stdint.h>
#include <sstream>
#include <string>

namespace ola {
namespace network {

/**
 * @addtogroup network
 * @{
 */

/**
 * @brief Represents a MAC Address.
 *
 * All methods use network byte order unless otherwise mentioned.
 * TODO(Peter): Is the above actually true for MAC addresses?
 */
class MACAddress {
 public:
  enum { LENGTH = 6 };

  MACAddress();

  /**
   * @brief Construct a new MAC address from binary data.
   * @param address a pointer to the memory containing the MAC address data. The
   * data should be most significant byte first.
   */
  explicit MACAddress(const uint8_t *address);

  MACAddress(const MACAddress &other);

  MACAddress& operator=(const MACAddress &other);

  bool operator==(const MACAddress &other) const;

  bool operator!=(const MACAddress &other) const {
    return !(*this == other);
  }

  /**
   * @brief Order addresses. Note that this won't order how humans expect
   * because ether_addr is in network byte order.
   * TODO(Peter): Check if this is actually true for MAC Addresses
   */
  bool operator<(const MACAddress &other) const;

  bool operator>(const MACAddress &other) const;

  // copy the address in network byte order to a location. The location
  // should be at least LENGTH bytes.
  void Get(uint8_t ptr[LENGTH]) const;

  /**
   * @brief Write the binary representation of the MAC address to memory.
   * @param buffer a pointer to memory to write the MAC address to
   * @param length the size of the memory block, should be at least LENGTH.
   * @returns true if length was >= LENGTH, false otherwise.
   */
  bool Pack(uint8_t *buffer, unsigned int length) const {
    if (length < LENGTH)
      return false;
    Get(buffer);
    return true;
  }

  /**
   * @brief Convert a mac address to a human readable string
   * @return a string
   */
  std::string ToString() const;

  friend std::ostream& operator<< (std::ostream &out,
                                   const MACAddress &address) {
    return out << address.ToString();
  }

  /**
   * @brief Convert a string to a MACAddress object
   * @param address a string in the form 'nn:nn:nn:nn:nn:nn' or
   * 'nn.nn.nn.nn.nn.nn'
   * @return a pointer to a MACAddress object if it worked, NULL otherwise
   */
  static MACAddress* FromString(const std::string &address);

  /**
   * @brief Convert a string to a MACAddress object
   * @param address a string in the form 'nn:nn:nn:nn:nn:nn' or
   * 'nn.nn.nn.nn.nn.nn'
   * @param[out] target a pointer to a MACAddress object
   * @return true if it worked, false otherwise
   */
  static bool FromString(const std::string &address, MACAddress *target);

  // useful for testing
  static MACAddress FromStringOrDie(const std::string &address);

 private:
  uint8_t m_address[LENGTH];
};
/**
 * @}
 */
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_MACADDRESS_H_
