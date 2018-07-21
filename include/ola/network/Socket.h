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
 * Socket.h
 * The UDP Socket interfaces
 * Copyright (C) 2005 Simon Newton
 *
 * UDPSocket, allows sending and receiving UDP datagrams
 */

#ifndef INCLUDE_OLA_NETWORK_SOCKET_H_
#define INCLUDE_OLA_NETWORK_SOCKET_H_

#include <stdint.h>

// On MinGW, SocketAddress.h pulls in WinSock2.h, which needs to be after
// WinSock2.h, hence this order
#include <ola/network/SocketAddress.h>

#include <ola/Callback.h>
#include <ola/base/Macro.h>
#include <ola/io/Descriptor.h>
#include <ola/io/IOQueue.h>
#include <ola/network/IPV4Address.h>
#include <string>

namespace ola {
namespace network {

/**
 * @brief The interface for UDPSockets.
 *
 * This only supports IPv4 sockets. Its an %Interface so we can mock it out for
 * testing.
 */
class UDPSocketInterface: public ola::io::BidirectionalFileDescriptor {
 public:
  UDPSocketInterface(): ola::io::BidirectionalFileDescriptor() {}
  ~UDPSocketInterface() {}

  /**
   * @brief Initialize the socket.
   * @returns false if initialization failed.
   */
  virtual bool Init() = 0;

  /**
   * @brief Bind this socket to an external address:port
   * @param endpoint the local socket address to bind to.
   * @returns true if the bind succeeded, false if it failed.
   */
  virtual bool Bind(const IPV4SocketAddress &endpoint) = 0;

  /**
   * @brief Return the local address this socket is bound to.
   * @param[out] address the local socket address this socket is bound to.
   * @returns true if the call succeeded, false if it failed.
   */
  virtual bool GetSocketAddress(IPV4SocketAddress *address) const = 0;

  /**
   * @brief Close the socket.
   * @returns true if the call succeeded, false if it failed.
   */
  virtual bool Close() = 0;

  virtual ola::io::DescriptorHandle ReadDescriptor() const = 0;
  virtual ola::io::DescriptorHandle WriteDescriptor() const = 0;

  /**
   * @brief Send data on this UDPSocket.
   * @param buffer the data to send
   * @param size the length of the data
   * @param ip the IP to send to
   * @param port the port to send to in HOST byte order.
   * @return the number of bytes sent.
   *
   * @deprecated Use the IPV4SocketAddress version instead.
   */
  virtual ssize_t SendTo(const uint8_t *buffer,
                         unsigned int size,
                         const IPV4Address &ip,
                         unsigned short port) const = 0;

  /**
   * @brief Send data on this UDPSocket.
   * @param buffer the data to send
   * @param size the length of the data
   * @param dest the IP:Port to send the datagram to.
   * @return the number of bytes sent
   */
  virtual ssize_t SendTo(const uint8_t *buffer,
                         unsigned int size,
                         const IPV4SocketAddress &dest) const = 0;

  /**
   * @brief Send data from an IOVecInterface.
   * @param data the IOVecInterface class to send.
   * @param ip the IP to send to
   * @param port the port to send to in HOST byte order.
   * @return the number of bytes sent.
   *
   * @deprecated Use the IPV4SocketAddress version instead.
   *
   * This will try to send as much data as possible.
   * If the data exceeds the MTU the UDP packet will probably get fragmented at
   * the IP layer (depends on OS really). Try to avoid this.
   */
  virtual ssize_t SendTo(ola::io::IOVecInterface *data,
                         const IPV4Address &ip,
                         unsigned short port) const = 0;

  /**
   * @brief Send data from an IOVecInterface.
   * @param data the IOVecInterface class to send.
   * @param dest the IPV4SocketAddress to send to
   * @return the number of bytes sent.
   *
   * This will try to send as much data as possible.
   * If the data exceeds the MTU the UDP packet will probably get fragmented at
   * the IP layer (depends on OS really). Try to avoid this.
   */
  virtual ssize_t SendTo(ola::io::IOVecInterface *data,
                         const IPV4SocketAddress &dest) const = 0;

  /**
   * @brief Receive data
   * @param buffer the buffer to store the data
   * @param data_read the size of the buffer, updated with the number of bytes
   * read
   * @return true or false
   */
  virtual bool RecvFrom(uint8_t *buffer, ssize_t *data_read) const = 0;

  /**
   * @brief Receive data
   * @param buffer the buffer to store the data
   * @param data_read the size of the buffer, updated with the number of bytes
   * read
   * @param source the src ip of the packet
   * @return true or false
   *
   * @deprecated Use the IPV4SocketAddress version instead.
   */
  virtual bool RecvFrom(
      uint8_t *buffer,
      ssize_t *data_read,
      IPV4Address &source) const = 0;  // NOLINT(runtime/references)

  /**
   * @brief Receive data and record the src address & port
   * @param buffer the buffer to store the data
   * @param data_read the size of the buffer, updated with the number of bytes
   * read
   * @param source the src ip of the packet
   * @param port the src port of the packet in host byte order
   * @return true or false
   *
   * @deprecated Use the IPV4SocketAddress version instead.
   */
  virtual bool RecvFrom(
      uint8_t *buffer,
      ssize_t *data_read,
      IPV4Address &source,  // NOLINT(runtime/references)
      uint16_t &port) const = 0;  // NOLINT(runtime/references)

  /**
   * @brief Receive a datagram on the UDP Socket.
   * @param buffer the buffer to store the data
   * @param data_read the size of the buffer, updated with the number of bytes
   * read
   * @param source the source of the datagram.
   * @return true or false
   */
  virtual bool RecvFrom(uint8_t *buffer,
                        ssize_t *data_read,
                        IPV4SocketAddress *source) = 0;

  /**
   * @brief Enable broadcasting for this socket.
   * @return true if it worked, false otherwise
   */
  virtual bool EnableBroadcast() = 0;

  /**
   * @brief Set the outgoing interface to be used for multicast transmission.
   * @param iface the address of the interface to use.
   */
  virtual bool SetMulticastInterface(const IPV4Address &iface) = 0;

  /**
   * @brief Join a multicast group
   * @param iface the address of the interface to use.
   * @param group the address of the group to join
   * @param multicast_loop enable multicast loop
   * @return true if it worked, false otherwise
   */
  virtual bool JoinMulticast(const IPV4Address &iface,
                             const IPV4Address &group,
                             bool multicast_loop = false) = 0;

  /**
   * @brief Leave a multicast group
   * @param iface the address of the interface to use.
   * @param group the address of the group to join
   * @return true if it worked, false otherwise
   */
  virtual bool LeaveMulticast(const IPV4Address &iface,
                              const IPV4Address &group) = 0;

  /**
   * @brief Set the tos field for a socket
   * @param tos the tos field
   * @return true if it worked, false otherwise
   */
  virtual bool SetTos(uint8_t tos) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(UDPSocketInterface);
};


/*
 * A UDPSocket (non connected)
 */
class UDPSocket: public UDPSocketInterface {
 public:
  UDPSocket()
      : UDPSocketInterface(),
        m_handle(ola::io::INVALID_DESCRIPTOR),
        m_bound_to_port(false) {}
  ~UDPSocket() { Close(); }
  bool Init();
  bool Bind(const IPV4SocketAddress &endpoint);

  bool GetSocketAddress(IPV4SocketAddress *address) const;

  bool Close();
  ola::io::DescriptorHandle ReadDescriptor() const { return m_handle; }
  ola::io::DescriptorHandle WriteDescriptor() const { return m_handle; }
  ssize_t SendTo(const uint8_t *buffer,
                 unsigned int size,
                 const IPV4Address &ip,
                 unsigned short port) const;
  ssize_t SendTo(const uint8_t *buffer,
                 unsigned int size,
                 const IPV4SocketAddress &dest) const;
  ssize_t SendTo(ola::io::IOVecInterface *data,
                 const IPV4Address &ip,
                 unsigned short port) const;
  ssize_t SendTo(ola::io::IOVecInterface *data,
                 const IPV4SocketAddress &dest) const;

  bool RecvFrom(uint8_t *buffer, ssize_t *data_read) const;
  bool RecvFrom(uint8_t *buffer,
                ssize_t *data_read,
                IPV4Address &source) const;  // NOLINT(runtime/references)
  bool RecvFrom(uint8_t *buffer,
                ssize_t *data_read,
                IPV4Address &source,  // NOLINT(runtime/references)
                uint16_t &port) const;  // NOLINT(runtime/references)

  bool RecvFrom(uint8_t *buffer,
                ssize_t *data_read,
                IPV4SocketAddress *source);

  bool EnableBroadcast();
  bool SetMulticastInterface(const IPV4Address &iface);
  bool JoinMulticast(const IPV4Address &iface,
                     const IPV4Address &group,
                     bool multicast_loop = false);
  bool LeaveMulticast(const IPV4Address &iface,
                      const IPV4Address &group);

  bool SetTos(uint8_t tos);

 private:
  ola::io::DescriptorHandle m_handle;
  bool m_bound_to_port;

  DISALLOW_COPY_AND_ASSIGN(UDPSocket);
};
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_SOCKET_H_
