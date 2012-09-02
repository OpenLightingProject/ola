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
 * SLPPacketBuilder.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_SLPPACKETBUILDER_H_
#define TOOLS_SLP_SLPPACKETBUILDER_H_

#include <ola/io/IOQueue.h>

#include <string>
#include <vector>

#include "tools/slp/SLPPacketConstants.h"

using ola::io::IOQueue;
using std::string;
using std::vector;

namespace ola {
namespace slp {

/**
 * The SLPPacketBuilder constructs packets.
 */
class SLPPacketBuilder {
  public:
    SLPPacketBuilder() {}
    ~SLPPacketBuilder() {}

    static void BuildDAAdvert(IOQueue *output,
                              xid_t xid,
                              bool multicast,
                              uint16_t error_code,
                              uint32_t boot_timestamp,
                              const string &url,
                              const string &scope_list);

  private:
    static void BuildSLPHeader(IOQueue *output,
                               slp_function_id_t function_id,
                               unsigned int length,
                               uint8_t flags,
                               xid_t xid);
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPPACKETBUILDER_H_
