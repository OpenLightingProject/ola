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
 * IPV4Address.h
 * Represents a IPv4 Address
 * Copyright (C) 2011 Simon Newton
 */

/**
 * @defgroup network Network
 * @brief Network related code.
 */

/**
 * @addtogroup network
 * @{
 * @file IPV4Address.h
 * @brief Represents an IPv4 Address.
 * @}
 */

#ifndef INCLUDE_OLA_NETWORK_IPV4ADDRESS_H_
#define INCLUDE_OLA_NETWORK_IPV4ADDRESS_H_

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
 * @brief Represents a IPv4 Address.
 *
 * All methods use network byte order unless otherwise mentioned.
 */
class IPV4Address {
 public:
    /**
     * @brief The length in bytes of an IPv4 address.
     */
    enum { LENGTH = 4 };

    /**
     * @brief Create a new IPv4 Address set to INADDR_ANY (0.0.0.0).
     */
    IPV4Address() {
      m_address = 0;
    }

    /**
     * @brief Create a new IPv4 Address from an uint32.
     * @param address the ip address, in network byte order.
     */
    explicit IPV4Address(uint32_t address) {
      m_address = address;
    }

    /**
     * @brief Copy constructor.
     * @param other the IPV4Address to copy.
     */
    IPV4Address(const IPV4Address &other)
        : m_address(other.m_address) {
    }

    /**
     * @brief Assignment operator.
     * @param other the IPV4Address to assign to this object.
     */
    IPV4Address& operator=(const IPV4Address &other) {
      if (this != &other) {
        m_address = other.m_address;
      }
      return *this;
    }

    /**
     * @brief Equals operator.
     * @param other the IPV4Address to compare.
     * @returns true if both IPV4Addresses are equal.
     */
    bool operator==(const IPV4Address &other) const {
      return m_address == other.m_address;
    }

    /**
     * @brief Not equals operator.
     * @param other the IPV4Address to compare.
     * @returns false if both IPV4Addresses are equal.
     */
    bool operator!=(const IPV4Address &other) const {
      return !(*this == other);
    }

    /**
     * @brief Less than operator for partial ordering.
     */
    bool operator<(const IPV4Address &other) const;

    /**
     * @brief Greater than operator.
     */
    bool operator>(const IPV4Address &other) const;

    /**
     * @brief Return the IPV4Address as an int in network-byte order.
     * @returns An uint32 representing the IP address.
     */
    uint32_t AsInt() const { return m_address; }

    /**
     * @brief Checks if this address is the wildcard address (0.0.0.0).
     * @returns true if this address is the wildcard address.
     */
    bool IsWildcard() const;

    /**
     * @brief Copy the IPV4Address to a memory location.
     * @param ptr the memory location to copy the address to. The location
     * should be at least LENGTH bytes.
     * @note The address is copied in network byte order.
     */
    void Get(uint8_t ptr[LENGTH]) const {
      memcpy(ptr, reinterpret_cast<const uint8_t*>(&m_address), LENGTH);
    }

    /**
     * @brief Convert the IPV4Address to a string.
     * @returns the string representation of this IPV4Address.
     */
    std::string ToString() const;

    /**
     * @brief Write the string representation of this IPV4Address to an
     * ostream.
     * @param out the ostream to write to.
     * @param address the address to write.
     */
    friend std::ostream& operator<<(std::ostream &out,
                                    const IPV4Address &address) {
      return out << address.ToString();
    }

    /**
     * @brief Convert a string to an IPV4Address.
     * @param address the IP address string to convert.
     * @returns a new IPV4Address or NULL if the string was invalid. The caller
     * is responsible for deleting the IPV4Address object.
     */
    static IPV4Address* FromString(const std::string &address);

    /**
     * @brief Convert a string to an IPV4Address.
     * @param address the IP address string to convert.
     * @param[out] target the converted IPV4Address.
     * @returns true if the string was a valid IPv4 address, false otherwise.
     */
    static bool FromString(const std::string &address, IPV4Address *target);

    /**
     * @brief Convert a string to an IPV4Address or abort.
     * @note This should only be used within tests.
     * @param address the IP address to convert.
     * @return an IPV4Address matching the string.
     */
    static IPV4Address FromStringOrDie(const std::string &address);

    /**
     * @brief Convert a subnet mask to its CIDR format value
     * @param address the subnet mask as an IPV4Address object
     * @param mask the mask variable to populate
     * @return true if we managed to convert the address to a CIDR value, false
         otherwise
     */
    static bool ToCIDRMask(IPV4Address address, uint8_t *mask);

    /**
     * @brief Returns the wildcard address INADDR_ANY (0.0.0.0).
     * @return an IPV4Address representing the wildcard address.
     */
    static IPV4Address WildCard();

    /**
     * @brief Returns the broadcast address INADDR_NONE (255.255.255.255).
     * @return an IPV4Address representing the broadcast address.
     */
    static IPV4Address Broadcast();

    /**
     * @brief Returns the loopback address (127.0.0.1).
     * @return an IPV4Address representing the loopback address.
     */
    static IPV4Address Loopback();

 private:
    uint32_t m_address;
};
/**
 * @}
 */
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_IPV4ADDRESS_H_
