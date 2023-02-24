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
 * RootHeader.h
 * Interface for the RootHeader class
 * Copyright (C) 2007 Simon Newton
 */

#ifndef LIBS_ACN_ROOTHEADER_H_
#define LIBS_ACN_ROOTHEADER_H_

#include "ola/acn/CID.h"

namespace ola {
namespace acn {

/*
 * The header for the root layer
 */
class RootHeader {
 public:
    RootHeader() {}
    ~RootHeader() {}
    void SetCid(ola::acn::CID cid) { m_cid = cid; }
    ola::acn::CID GetCid() const { return m_cid; }

    bool operator==(const RootHeader &other) const {
      return m_cid == other.m_cid;
    }
 private:
    ola::acn::CID m_cid;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_ROOTHEADER_H_
