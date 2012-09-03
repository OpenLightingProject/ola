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
 * SLPPacketParser.cpp
 * Copyright (C) 2012 Simon Newton
 */


#include <string>
#include <vector>
#include <ola/Logging.h>

#include "tools/slp/SLPPacketParser.h"

using ola::io::IOQueue;
using ola::network::IPV4Address;
using std::string;
using std::vector;

namespace ola {
namespace slp {

/*
 * Return the function-id for a packet, or 0 if the packet is malformed.
 */
uint8_t SLPPacketParser::DetermineFunctionID(const uint8_t *data,
                                             unsigned int length) {
  if (length < 2) {
    OLA_WARN << "SLP Packet too short to extract function-id";
    return 0;
  }
  return data[1];
};
}  // slp
}  // ola
