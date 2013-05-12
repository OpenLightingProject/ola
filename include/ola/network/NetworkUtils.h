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
 * NetworkUtils.h
 * Abstract various network functions.
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef INCLUDE_OLA_NETWORK_NETWORKUTILS_H_
#define INCLUDE_OLA_NETWORK_NETWORKUTILS_H_

#ifdef WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include <ola/network/Interface.h>
#include <string>


namespace ola {
namespace network {

bool StringToAddress(const std::string &address, struct in_addr &addr);
std::string AddressToString(const struct in_addr &addr);

std::string HardwareAddressToString(uint8_t hw_address[MAC_LENGTH]);

bool IsBigEndian();

// we define uint8_t versions of these so we can call them with any type.
uint8_t NetworkToHost(uint8_t value);
uint16_t NetworkToHost(uint16_t value);
uint32_t NetworkToHost(uint32_t value);
int8_t NetworkToHost(int8_t value);
int16_t NetworkToHost(int16_t value);
int32_t NetworkToHost(int32_t value);
uint8_t HostToNetwork(uint8_t value);
uint16_t HostToNetwork(uint16_t value);
uint32_t HostToNetwork(uint32_t value);
int8_t HostToNetwork(int8_t value);
int16_t HostToNetwork(int16_t value);
int32_t HostToNetwork(int32_t value);

uint8_t HostToLittleEndian(uint8_t value);
uint16_t HostToLittleEndian(uint16_t value);
uint32_t HostToLittleEndian(uint32_t value);
int8_t HostToLittleEndian(int8_t value);
int16_t HostToLittleEndian(int16_t value);
int32_t HostToLittleEndian(int32_t value);
uint8_t LittleEndianToHost(uint8_t value);
uint16_t LittleEndianToHost(uint16_t value);
uint32_t LittleEndianToHost(uint32_t value);
int8_t LittleEndianToHost(int8_t value);
int16_t LittleEndianToHost(int16_t value);
int32_t LittleEndianToHost(int32_t value);

std::string FullHostname();
std::string Hostname();
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_NETWORKUTILS_H_

