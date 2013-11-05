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
 * Interface.cpp
 * Represents network interface.
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string.h>
#include <string>
#include <vector>

#include "ola/StringUtils.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/NetworkUtils.h"

#ifdef WIN32
#include "common/network/WindowsInterfacePicker.h"
#else
#include "common/network/PosixInterfacePicker.h"
#endif

namespace ola {
namespace network {

using std::string;
using std::vector;


Interface::Interface()
    : loopback(false) {
}


Interface::Interface(const string &name,
                     const IPV4Address &ip_address,
                     const IPV4Address &broadcast_address,
                     const IPV4Address &subnet_mask,
                     const MACAddress &hw_address,
                     bool loopback)
    : name(name),
      ip_address(ip_address),
      bcast_address(broadcast_address),
      subnet_mask(subnet_mask),
      hw_address(hw_address),
      loopback(loopback) {
}


Interface::Interface(const Interface &other)
    : name(other.name),
      ip_address(other.ip_address),
      bcast_address(other.bcast_address),
      subnet_mask(other.subnet_mask),
      hw_address(other.hw_address),
      loopback(other.loopback) {
}


Interface& Interface::operator=(const Interface &other) {
  if (this != &other) {
    name = other.name;
    ip_address = other.ip_address;
    bcast_address = other.bcast_address;
    subnet_mask = other.subnet_mask;
    hw_address = other.hw_address;
    loopback = other.loopback;
  }
  return *this;
}


bool Interface::operator==(const Interface &other) {
  return (name == other.name &&
          ip_address == other.ip_address &&
          subnet_mask == other.subnet_mask &&
          loopback == other.loopback);
}


/**
 * Create a new interface builder
 */
InterfaceBuilder::InterfaceBuilder()
  : m_ip_address(0),
    m_broadcast_address(0),
    m_subnet_mask(0),
    m_hw_address() {
}


/**
 * Set the address of the interface to build.
 */
bool InterfaceBuilder::SetAddress(const string &ip_address) {
  return SetAddress(ip_address, &m_ip_address);
}


/**
 * Set the broadcast address of the interface to build.
 */
bool InterfaceBuilder::SetBroadcast(const string &broadcast_address) {
  return SetAddress(broadcast_address, &m_broadcast_address);
}


/**
 * Set the subnet mask of the interface to build.
 */
bool InterfaceBuilder::SetSubnetMask(const string &mask) {
  return SetAddress(mask, &m_subnet_mask);
}


/**
 * Set the loopback flag.
 */
void InterfaceBuilder::SetLoopback(bool loopback) {
  m_loopback = loopback;
}


/**
 * Reset the builder object
 */
void InterfaceBuilder::Reset() {
  m_name = "";
  m_ip_address = IPV4Address(0);
  m_broadcast_address = IPV4Address(0);
  m_subnet_mask = IPV4Address(0);
  m_hw_address = MACAddress();
  m_loopback = false;
}


/**
 * Return a new interface object.
 * Maybe in the future we should check that the broadcast address, ip address
 * and netmask are consistent. We could even infer the broadcast_address if it
 * isn't provided.
 */
Interface InterfaceBuilder::Construct() {
  return Interface(m_name,
                   m_ip_address,
                   m_broadcast_address,
                   m_subnet_mask,
                   m_hw_address,
                   m_loopback);
}


/**
 * Set a IPV4Address object from a string
 */
bool InterfaceBuilder::SetAddress(const string &str, IPV4Address *target) {
  IPV4Address tmp_address;
  if (!IPV4Address::FromString(str, &tmp_address))
    return false;
  *target = tmp_address;
  return true;
}
}  // namespace network
}  // namespace ola
