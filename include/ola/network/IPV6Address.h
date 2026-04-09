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
 * IPV6Address.h
 * Represents an IPv6 Address
 * Copyright (C) 2023 Peter Newman
 */

/**
 * @addtogroup network
 * @{
 * @file IPV6Address.h
 * @brief Represents an IPv6 Address.
 * @}
 */

#ifndef INCLUDE_OLA_NETWORK_IPV6ADDRESS_H_
#define INCLUDE_OLA_NETWORK_IPV6ADDRESS_H_

#include <sys/socket.h>  // Required by FreeBSD
#include <netinet/in.h>  // Required by FreeBSD
#include <stdint.h>
#include <string.h>
#include <sstream>
#include <string>

namespace ola {
namespace network {

/**
 * @addtogroup network
 * @{
 */

/**
 * @brief Represents an IPv6 Address.
 *
 * All methods use network byte order unless otherwise mentioned.
 */
class IPV6Address {
 public:
  /**
   * @brief The length in bytes of an IPv6 address.
   */
  enum { LENGTH = 16 };

  /**
   * @brief Create a new IPv6 Address set to IN6ADDR_ANY_INIT (::).
   */
  IPV6Address() {
    m_address = IN6ADDR_ANY_INIT;
  }

// TODO(Peter): From uint128_t?

  /**
   * @brief Construct a new IPv6 address from binary data.
   * @param address a pointer to the memory containing the IPv6 address data. The
   * data should be most significant byte first.
   */
  explicit IPV6Address(const uint8_t *address);

  /**
   * @brief Create a new IPv6 Address from a in6_addr struct.
   * @param address the ip address, in network byte order.
   */
  explicit IPV6Address(in6_addr address) {
    m_address = address;
  }

  /**
   * @brief Copy constructor.
   * @param other the IPV6Address to copy.
   */
  IPV6Address(const IPV6Address &other)
      : m_address(other.m_address) {
  }

  /**
   * @brief Assignment operator.
   * @param other the IPV6Address to assign to this object.
   */
  IPV6Address& operator=(const IPV6Address &other) {
    if (this != &other) {
      m_address = other.m_address;
    }
    return *this;
  }

  /**
   * @brief Equals operator.
   * @param other the IPV6Address to compare.
   * @returns true if both IPV6Addresses are equal.
   */
  bool operator==(const IPV6Address &other) const {
    return IN6_ARE_ADDR_EQUAL(&m_address, &other.m_address);
  }

  /**
   * @brief Not equals operator.
   * @param other the IPV6Address to compare.
   * @returns false if both IPV6Addresses are equal.
   */
  bool operator!=(const IPV6Address &other) const {
    return !(*this == other);
  }

  /**
   * @brief Less than operator for partial ordering.
   */
  bool operator<(const IPV6Address &other) const;

  /**
   * @brief Greater than operator.
   */
  bool operator>(const IPV6Address &other) const;

  /**
//   * @brief Return the IPV6Address as an int in network-byte order.
//   * @returns An uint32 representing the IP address.
   */
//  uint32_t AsInt() const { return m_address; }

  /**
   * @brief Checks if this address is the wildcard address (::).
   * @returns true if this address is the wildcard address.
   */
  bool IsWildcard() const;

  /**
   * @brief Copy the IPV6Address to a memory location.
   * @param ptr the memory location to copy the address to. The location
   * should be at least LENGTH bytes.
   * @note The address is copied in network byte order.
   */
  void Get(uint8_t ptr[LENGTH]) const {
    memcpy(ptr,
           reinterpret_cast<const uint8_t*>(&m_address.s6_addr[0]),
           LENGTH);
  }

  /**
   * @brief Write the binary representation of the IPv6 address to memory.
   * @param buffer a pointer to memory to write the IPv6 address to
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
   * @brief Convert the IPV6Address to a string.
   * @returns the string representation of this IPV6Address.
   */
  std::string ToString() const;

  /**
   * @brief Write the string representation of this IPV6Address to an
   * ostream.
   * @param out the ostream to write to.
   * @param address the address to write.
   */
  friend std::ostream& operator<<(std::ostream &out,
                                  const IPV6Address &address) {
    return out << address.ToString();
  }

  /**
   * @brief Convert a string to an IPV6Address.
   * @param address the IP address string to convert.
   * @returns a new IPV6Address or NULL if the string was invalid. The caller
   * is responsible for deleting the IPV6Address object.
   */
  static IPV6Address* FromString(const std::string &address);

  /**
   * @brief Convert a string to an IPV6Address.
   * @param address the IP address string to convert.
   * @param[out] target the converted IPV6Address.
   * @returns true if the string was a valid IPv6 address, false otherwise.
   */
  static bool FromString(const std::string &address, IPV6Address *target);

  /**
   * @brief Convert a string to an IPV6Address or abort.
   * @note This should only be used within tests.
   * @param address the IP address to convert.
   * @return an IPV6Address matching the string.
   */
  static IPV6Address FromStringOrDie(const std::string &address);

  /**
//   * @brief Convert a subnet mask to its CIDR format value
//   * @param address the subnet mask as an IPV6Address object
   * @param mask the mask variable to populate
   * @return true if we managed to convert the address to a CIDR value, false
       otherwise
   */
//  static bool ToCIDRMask(IPV6Address address, uint8_t *mask);

  /**
   * @brief Returns the wildcard address IN6ADDR_ANY_INIT (::).
   * @return an IPV6Address representing the wildcard address.
   */
  static IPV6Address WildCard();

  // TODO(Peter): Add support for the all-nodes link-local multicast group

  /**
   * @brief Returns the loopback address (::1/128).
   * @return an IPV6Address representing the loopback address.
   */
  static IPV6Address Loopback();

 private:
  // TODO(Peter): Decide how to store the address internally...
  in6_addr m_address;
};
/**
 * @}
 */
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_IPV6ADDRESS_H_
