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
 * HeaderSet.h
 * Interface for the HeaderSet class
 * HeaderSet is passed down the parsing stack and contains a collection of PDU
 * headers
 * Copyright (C) 2007-2009 Simon Newton
 */

#ifndef PLUGINS_E131_E131_HEADERSET_H_
#define PLUGINS_E131_E131_HEADERSET_H_

#include "plugins/e131/e131/RootHeader.h"
#include "plugins/e131/e131/E131Header.h"
#include "plugins/e131/e131/DMPHeader.h"

namespace ola {
namespace plugin {
namespace e131 {

class HeaderSet {
  public:
    HeaderSet() {}
    ~HeaderSet() {}

    const RootHeader &GetRootHeader() const { return m_root_header; }
    void SetRootHeader(RootHeader &header) { m_root_header = header; }

    const E131Header &GetE131Header() const { return m_e131_header; }
    void SetE131Header(E131Header &header) { m_e131_header = header; }

    const DMPHeader &GetDMPHeader() const { return m_dmp_header; }
    void SetDMPHeader(DMPHeader &header) { m_dmp_header = header; }

    bool operator==(const HeaderSet &other) const {
      return m_root_header == other.m_root_header  &&
      m_e131_header == other.m_e131_header &&
       m_dmp_header == other.m_dmp_header;
    }

  private:
    RootHeader m_root_header;
    E131Header m_e131_header;
    DMPHeader m_dmp_header;
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_HEADERSET_H_
