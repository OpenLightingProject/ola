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
 * LLRPProbeReplyInflator.cpp
 * The Inflator for the LLRP Probe Reply PDUs
 * Copyright (C) 2020 Peter Newman
 */

#include <iostream>
#include <memory>
#include "ola/Logging.h"
#include "include/ola/rdm/UID.h"
#include "include/ola/rdm/UIDSet.h"
#include "include/ola/strings/Format.h"
#include "libs/acn/LLRPProbeReplyInflator.h"
#include "libs/acn/LLRPProbeReplyPDU.h"

namespace ola {
namespace acn {

using ola::acn::LLRPProbeReplyPDU;
using ola::rdm::UID;
using ola::rdm::UIDSet;

/**
 * Create a new LLRP Probe Reply inflator
 */
LLRPProbeReplyInflator::LLRPProbeReplyInflator()
    : BaseInflator(PDU::ONE_BYTE) {
}

/**
 * Set an LLRPProbeReplyHandler to run when receiving an LLRP Probe Reply
 * message.
 * @param handler the callback to invoke when there is and LLRP Probe Reply.
 */
void LLRPProbeReplyInflator::SetLLRPProbeReplyHandler(
    LLRPProbeReplyHandler *handler) {
  m_llrp_probe_reply_handler.reset(handler);
}


/*
 * Decode the LLRP Probe Reply 'header', which is 0 bytes in length.
 * @param headers the HeaderSet to add to
 * @param data a pointer to the data
 * @param length length of the data
 * @returns true if successful, false otherwise
 */
bool LLRPProbeReplyInflator::DecodeHeader(HeaderSet *,
                                          const uint8_t*,
                                          unsigned int,
                                          unsigned int *bytes_used) {
  *bytes_used = 0;
  return true;
}


/*
 * Handle a LLRP Probe Reply PDU for E1.33.
 */
bool LLRPProbeReplyInflator::HandlePDUData(uint32_t vector,
                                           const HeaderSet &headers,
                                           const uint8_t *data,
                                           unsigned int pdu_len) {
  if (vector != LLRPProbeReplyPDU::VECTOR_PROBE_REPLY_DATA) {
    OLA_INFO << "Not a probe reply, vector was " << vector;
    return true;
  }

  ola::strings::FormatData(&std::cout, data, pdu_len);

  LLRPProbeReplyPDU::llrp_probe_reply_pdu_data pdu_data;
  if (pdu_len > sizeof(pdu_data)) {
    OLA_WARN << "Got too much data, received " << pdu_len << " only expecting "
             << sizeof(pdu_data);
    return false;
  }

  memcpy(reinterpret_cast<uint8_t*>(&pdu_data), data, sizeof(pdu_data));

  OLA_DEBUG << "Probe from " << UID(pdu_data.target_uid);

  LLRPProbeReply reply(UID(pdu_data.target_uid));

  if (m_llrp_probe_reply_handler.get()) {
    m_llrp_probe_reply_handler->Run(&headers,
                                    reply);
  } else {
    OLA_WARN << "No LLRP Probe Reply handler defined!";
  }
  return true;
}
}  // namespace acn
}  // namespace ola
