/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * TransportHeader.h
 * Interface for the TransportHeader class.
 * This holds the source IP of the packet which is used to address replies
 * correctly. At some point in the future we should try to abtract the
 * transport protocol (IP/UDP in this case).
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_E131_E131_TRANSPORTHEADER_H_
#define PLUGINS_E131_E131_TRANSPORTHEADER_H_

#include "ola/network/IPV4Address.h"

namespace ola {
namespace plugin {
namespace e131 {

/*
 * The header for the transport layer
 */
class TransportHeader {
  public:
    TransportHeader() {}

    explicit TransportHeader(const ola::network::IPV4Address &src_ip)
        : m_src_ip(src_ip) {}

    ~TransportHeader() {}
    const ola::network::IPV4Address& SourceIP() const { return m_src_ip; }

    bool operator==(const TransportHeader &other) const {
      return m_src_ip == other.m_src_ip;
    }
  private:
    ola::network::IPV4Address m_src_ip;
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_TRANSPORTHEADER_H_
