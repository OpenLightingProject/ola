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
 * RDMPDU.cpp
 * The RDMPDU
 * Copyright (C) 2012 Simon Newton
 */

#include "libs/acn/RDMPDU.h"

#include <ola/network/NetworkUtils.h>
#include <ola/rdm/RDMPacket.h>

namespace ola {
namespace acn {

using ola::io::OutputStream;
using ola::network::HostToNetwork;

void RDMPDU::PrependPDU(ola::io::IOStack *stack) {
  uint8_t vector = HostToNetwork(ola::rdm::START_CODE);
  stack->Write(reinterpret_cast<uint8_t*>(&vector), sizeof(vector));
  PrependFlagsAndLength(stack);
}
}  // namespace acn
}  // namespace ola
