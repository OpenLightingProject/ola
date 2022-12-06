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
 * Interface.h
 * Represents a network interface.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_NETWORK_INTERFACE_H_
#define INCLUDE_OLA_NETWORK_INTERFACE_H_

#include <stdint.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/MACAddress.h>
#include <string>

namespace ola {
namespace network {

/*
 * Represents an interface.
 */
class Interface {
 public:
  enum { DEFAULT_INDEX = -1 };

  Interface();
  Interface(const std::string &name,
            const IPV4Address &ip_address,
            const IPV4Address &broadcast_address,
            const IPV4Address &subnet_mask,
            const MACAddress &hw_address,
            bool loopback,
            int32_t index = DEFAULT_INDEX,
            uint16_t type = ARP_VOID_TYPE);
  Interface(const Interface &other);
  Interface& operator=(const Interface &other);
  bool operator==(const Interface &other) const;

  /**
   * @brief Convert the Interface to a string.
   * @param separator the separator to use between items, defaults to ", ".
   * @returns the string representation of this Interface.
   */
  std::string ToString(const std::string &separator = ", ") const;

  /**
   * @brief Write the string representation of this Interface to an
   * ostream.
   * @param out the ostream to write to.
   * @param iface the iface to write.
   */
  friend std::ostream& operator<<(std::ostream &out,
                                  const Interface &iface) {
    return out << iface.ToString();
  }

  std::string name;
  IPV4Address ip_address;
  IPV4Address bcast_address;
  IPV4Address subnet_mask;
  MACAddress hw_address;
  bool loopback;
  int32_t index;
  uint16_t type;

  /* Void type, nothing is known */
  static const uint16_t ARP_VOID_TYPE;
  static const uint16_t ARP_ETHERNET_TYPE;
};


/**
 * An InterfaceBuilder, this can construct Interface objects
 */
class InterfaceBuilder {
 public:
  InterfaceBuilder();
  ~InterfaceBuilder() {}

  void SetName(const std::string &name) { m_name = name; }

  bool SetAddress(const std::string &ip_address);
  void SetAddress(const IPV4Address &ip_address) {
    m_ip_address = ip_address;
  }

  bool SetBroadcast(const std::string &broadcast_address);
  void SetBroadcast(const IPV4Address &broadcast_address) {
    m_broadcast_address = broadcast_address;
  }

  bool SetSubnetMask(const std::string &mask);
  void SetSubnetMask(const IPV4Address &mask) {
    m_subnet_mask = mask;
  }

  void SetHardwareAddress(const MACAddress &mac_address) {
    m_hw_address = mac_address;
  }

  void SetLoopback(bool loopback);

  void SetIndex(int32_t index);

  void SetType(uint16_t type);

  void Reset();
  Interface Construct();

 private:
  std::string m_name;
  IPV4Address m_ip_address;
  IPV4Address m_broadcast_address;
  IPV4Address m_subnet_mask;
  MACAddress m_hw_address;
  bool m_loopback;
  int32_t m_index;
  uint16_t m_type;

  bool SetAddress(const std::string &str, IPV4Address *target);
};

// Sorts interfaces by index.
struct InterfaceIndexOrdering {
  inline bool operator() (const Interface &if1, const Interface &if2) {
    return (if1.index < if2.index);
  }
};
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_INTERFACE_H_
