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
 * BrokerNullPDU.cpp
 * The BrokerNullPDU
 * Copyright (C) 2023 Peter Newman
 */

#include "libs/acn/BrokerNullPDU.h"

#include <ola/network/NetworkUtils.h>
#include <ola/acn/ACNVectors.h>

namespace ola {
namespace acn {

using ola::io::OutputStream;
using ola::network::HostToNetwork;
using std::min;
using std::string;

void BrokerNullPDU::PrependPDU(ola::io::IOStack *stack) {
  uint16_t vector = HostToNetwork(static_cast<uint16_t>(VECTOR_BROKER_NULL));
  stack->Write(reinterpret_cast<uint8_t*>(&vector), sizeof(vector));
  PrependFlagsAndLength(stack, VFLAG_MASK | HFLAG_MASK | DFLAG_MASK, true);
}
}  // namespace acn
}  // namespace ola
