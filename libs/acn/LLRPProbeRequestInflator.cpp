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
 * LLRPProbeRequestInflator.cpp
 * The Inflator for the LLRP Probe Request PDUs
 * Copyright (C) 2020 Peter Newman
 */

#include <iostream>
#include <memory>
#include "ola/Logging.h"
#include "include/ola/rdm/UID.h"
#include "include/ola/rdm/UIDSet.h"
#include "include/ola/strings/Format.h"
#include "libs/acn/LLRPProbeRequestInflator.h"
#include "libs/acn/LLRPProbeRequestPDU.h"

namespace ola {
namespace acn {

using ola::acn::LLRPProbeRequestPDU;
using ola::rdm::UID;
using ola::rdm::UIDSet;

/**
 * Create a new LLRP Probe Request inflator
 */
LLRPProbeRequestInflator::LLRPProbeRequestInflator()
    : BaseInflator(PDU::ONE_BYTE) {
}

/**
 * Set a RDMHandler to run when receiving a RDM message.
 * @param handler the callback to invoke when there is rdm data for this
 * universe.
 */
void LLRPProbeRequestInflator::SetLLRPProbeRequestHandler(
    LLRPProbeRequestHandler *handler) {
  m_llrp_probe_request_handler.reset(handler);
}


/*
 * Decode the LLRP Probe Request 'header', which is 0 bytes in length.
 * @param headers the HeaderSet to add to
 * @param data a pointer to the data
 * @param length length of the data
 * @returns true if successful, false otherwise
 */
bool LLRPProbeRequestInflator::DecodeHeader(HeaderSet *,
                                            const uint8_t*,
                                            unsigned int,
                                            unsigned int *bytes_used) {
  *bytes_used = 0;
  return true;
}


/*
 * Handle a LLRP Probe Request PDU for E1.33.
 */
bool LLRPProbeRequestInflator::HandlePDUData(uint32_t vector,
                                             const HeaderSet &headers,
                                             const uint8_t *data,
                                             unsigned int pdu_len) {
  if (vector != LLRPProbeRequestPDU::VECTOR_PROBE_REQUEST_DATA) {
    OLA_INFO << "Not a probe request, vector was " << vector;
    return true;
  }

  ola::strings::FormatData(&std::cout, data, pdu_len);

  LLRPProbeRequestPDU::llrp_probe_request_pdu_data pdu_data;
  if (pdu_len > sizeof(pdu_data)) {
    OLA_WARN << "Got too much data, received " << pdu_len << " only expecting "
             << sizeof(pdu_data);
    return false;
  }

  unsigned int known_uids_size = static_cast<unsigned int>(
      pdu_len - (sizeof(pdu_data) -
      sizeof(pdu_data.known_uids)));
  if (known_uids_size % UID::UID_SIZE != 0) {
    OLA_WARN << "Got a partial known UID, received " << known_uids_size
             << " bytes";
    return false;
  }

  memcpy(reinterpret_cast<uint8_t*>(&pdu_data), data, sizeof(pdu_data));

  OLA_DEBUG << "Probe from " << UID(pdu_data.lower_uid) << " to "
            << UID(pdu_data.upper_uid);

  LLRPProbeRequest request(UID(pdu_data.lower_uid), UID(pdu_data.upper_uid));
  request.client_tcp_connection_inactive =
      (pdu_data.filter &
       LLRPProbeRequestPDU::FILTER_CLIENT_TCP_CONNECTION_INACTIVE);
  request.brokers_only = (pdu_data.filter &
                          LLRPProbeRequestPDU::FILTER_BROKERS_ONLY);
  unsigned int known_uids_used_size = known_uids_size;
  request.known_uids = UIDSet(pdu_data.known_uids, &known_uids_used_size);

  if (m_llrp_probe_request_handler.get()) {
    m_llrp_probe_request_handler->Run(&headers,
                                      request);
  } else {
    OLA_WARN << "No LLRP Probe Request handler defined!";
  }
  return true;
}
}  // namespace acn
}  // namespace ola
