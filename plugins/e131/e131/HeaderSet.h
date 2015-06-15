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
 * HeaderSet.h
 * Interface for the HeaderSet class
 * HeaderSet is passed down the parsing stack and contains a collection of PDU
 * headers
 * Copyright (C) 2007 Simon Newton
 */

#ifndef PLUGINS_E131_E131_HEADERSET_H_
#define PLUGINS_E131_E131_HEADERSET_H_

#include "plugins/e131/e131/DMPHeader.h"
#include "plugins/e131/e131/E131Header.h"
#include "plugins/e131/e131/E133Header.h"
#include "plugins/e131/e131/RootHeader.h"
#include "plugins/e131/e131/TransportHeader.h"

namespace ola {
namespace plugin {
namespace e131 {

class HeaderSet {
 public:
    HeaderSet() {}
    ~HeaderSet() {}

    const TransportHeader &GetTransportHeader() const {
      return m_transport_header;
    }
    void SetTransportHeader(const TransportHeader &header) {
      m_transport_header = header;
    }

    const RootHeader &GetRootHeader() const { return m_root_header; }
    void SetRootHeader(const RootHeader &header) { m_root_header = header; }

    const E131Header &GetE131Header() const { return m_e131_header; }
    void SetE131Header(const E131Header &header) { m_e131_header = header; }

    const E133Header &GetE133Header() const { return m_e133_header; }
    void SetE133Header(const E133Header &header) { m_e133_header = header; }

    const DMPHeader &GetDMPHeader() const { return m_dmp_header; }
    void SetDMPHeader(const DMPHeader &header) { m_dmp_header = header; }

    bool operator==(const HeaderSet &other) const {
      return (
          m_transport_header == other.m_transport_header &&
          m_root_header == other.m_root_header  &&
          m_e131_header == other.m_e131_header &&
          m_e133_header == other.m_e133_header &&
          m_dmp_header == other.m_dmp_header);
    }

 private:
    TransportHeader m_transport_header;
    RootHeader m_root_header;
    E131Header m_e131_header;
    E133Header m_e133_header;
    DMPHeader m_dmp_header;
};
}  // namespace e131
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_E131_E131_HEADERSET_H_
