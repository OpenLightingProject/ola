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
 * Interface.h
 * Represents a network interface.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_NETWORK_INTERFACE_H_
#define INCLUDE_OLA_NETWORK_INTERFACE_H_

#ifdef WIN32
// TODO(Peter): Do something else
#else
#include <net/if_arp.h>
#endif

#include <stdint.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/MACAddress.h>
#include <string>

namespace ola {
namespace network {

using std::string;

/*
 * Represents an interface.
 */
class Interface {
  public:
    Interface();
    Interface(const string &name,
              const IPV4Address &ip_address,
              const IPV4Address &broadcast_address,
              const IPV4Address &subnet_mask,
              const MACAddress &hw_address,
              bool loopback,
              int32_t index = 0,
              uint16_t type = ARPHRD_VOID);
    Interface(const Interface &other);
    Interface& operator=(const Interface &other);
    bool operator==(const Interface &other);

    std::string name;
    IPV4Address ip_address;
    IPV4Address bcast_address;
    IPV4Address subnet_mask;
    MACAddress hw_address;
    bool loopback;
    int32_t index;
    uint16_t type;
};


/**
 * An InterfaceBuilder, this can construct Interface objects
 */
class InterfaceBuilder {
  public:
    InterfaceBuilder();
    ~InterfaceBuilder() {}

    void SetName(const string &name) { m_name = name; }

    bool SetAddress(const string &ip_address);
    void SetAddress(const IPV4Address &ip_address) {
      m_ip_address = ip_address;
    }

    bool SetBroadcast(const string &broadcast_address);
    void SetBroadcast(const IPV4Address &broadcast_address) {
      m_broadcast_address = broadcast_address;
    }

    bool SetSubnetMask(const string &mask);
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

    bool SetAddress(const string &str, IPV4Address *target);
};
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_INTERFACE_H_
