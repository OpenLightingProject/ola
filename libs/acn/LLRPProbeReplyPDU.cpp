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
 * LLRPProbeReplyPDU.cpp
 * The LLRPProbeReplyPDU
 * Copyright (C) 2020 Peter Newman
 */

#include "libs/acn/LLRPProbeReplyPDU.h"

#include <ola/network/MACAddress.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/UID.h>

namespace ola {
namespace acn {

using ola::io::OutputStream;
using ola::network::HostToNetwork;
using ola::network::MACAddress;
using ola::rdm::UID;

void LLRPProbeReplyPDU::PrependPDU(ola::io::IOStack *stack,
                                   const UID &target_uid,
                                   const MACAddress &hardware_address,
                                   const LLRPComponentType type) {
  llrp_probe_reply_pdu_data data;
  target_uid.Pack(data.target_uid, sizeof(data.target_uid));
  hardware_address.Pack(data.hardware_address, sizeof(data.hardware_address));
  data.type = HostToNetwork(static_cast<uint8_t>(type));
  stack->Write(reinterpret_cast<uint8_t*>(&data),
               static_cast<unsigned int>(sizeof(llrp_probe_reply_pdu_data)));
  uint8_t vector = HostToNetwork(VECTOR_PROBE_REPLY_DATA);
  stack->Write(reinterpret_cast<uint8_t*>(&vector), sizeof(vector));
  PrependFlagsAndLength(stack, VFLAG_MASK | HFLAG_MASK | DFLAG_MASK, true);
}
}  // namespace acn
}  // namespace ola
