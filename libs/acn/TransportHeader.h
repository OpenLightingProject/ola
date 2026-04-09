/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * TransportHeader.h
 * Interface for the TransportHeader class.
 * This holds the source IP of the packet which is used to address replies
 * correctly. At some point in the future we should try to abstract the
 * transport protocol (IP/UDP in this case).
 * Copyright (C) 2011 Simon Newton
 */

#ifndef LIBS_ACN_TRANSPORTHEADER_H_
#define LIBS_ACN_TRANSPORTHEADER_H_

#include "ola/network/SocketAddress.h"

namespace ola {
namespace acn {

/*
 * The header for the transport layer
 */
class TransportHeader {
 public:
  enum TransportType {
    TCP,
    UDP,
    UNDEFINED,
  };

  TransportHeader() : m_transport_type(UNDEFINED) {}

  TransportHeader(const TransportHeader& other)
      : m_source(other.m_source),
        m_transport_type(other.m_transport_type) {}

  TransportHeader(const ola::network::IPV4SocketAddress &source,
                  TransportType type)
      : m_source(source),
        m_transport_type(type) {}

  ~TransportHeader() {}
  const ola::network::IPV4SocketAddress& Source() const { return m_source; }
  TransportType Transport() const { return m_transport_type; }

  bool operator==(const TransportHeader &other) const {
    return (m_source == other.m_source &&
            m_transport_type == other.m_transport_type);
  }

  void operator=(const TransportHeader &other) {
    m_source = other.m_source;
    m_transport_type = other.m_transport_type;
  }

 private:
  ola::network::IPV4SocketAddress m_source;
  TransportType m_transport_type;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_TRANSPORTHEADER_H_
