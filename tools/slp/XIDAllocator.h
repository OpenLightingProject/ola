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
 * XIDAllocator.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_XIDALLOCATOR_H_
#define TOOLS_SLP_XIDALLOCATOR_H_

#include "tools/slp/SLPPacketConstants.h"

namespace ola {
namespace slp {

/**
 * XIDAllocator, this ensures that we increment the global xid whever we go to
 * use it.
 */
class XIDAllocator {
  public:
    XIDAllocator(): m_xid(0) { }
    explicit XIDAllocator(xid_t xid);

    xid_t Next() { return m_xid++; }

  private:
    xid_t m_xid;
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_XIDALLOCATOR_H_
