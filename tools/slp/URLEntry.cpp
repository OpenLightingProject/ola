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
 * URLEntry.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/io/OutputStream.h>
#include <ola/network/NetworkUtils.h>
#include <stdint.h>
#include <string>
#include "tools/slp/URLEntry.h"
#include "tools/slp/SLPPacketBuilder.h"


using std::string;
using ola::network::HostToNetwork;

namespace ola {
namespace slp {


void URLEntry::Write(ola::io::OutputStreamInterface *output) const {
  *output << static_cast<uint8_t>(0);  // reservered
  *output << HostToNetwork(m_lifetime);
  SLPPacketBuilder::WriteString(output, m_url);
  *output << static_cast<uint8_t>(0);  // # of URL auths
}
}  // slp
}  // ola
