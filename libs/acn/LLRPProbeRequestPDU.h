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
 * LLRPProbeRequestPDU.h
 * The LLRPProbeRequestPDU class
 * Copyright (C) 2020 Peter Newman
 */

#ifndef LIBS_ACN_LLRPPROBEREQUESTPDU_H_
#define LIBS_ACN_LLRPPROBEREQUESTPDU_H_

#include <ola/io/IOStack.h>
#include <ola/rdm/UID.h>
#include <ola/rdm/UIDSet.h>

#include "libs/acn/PDU.h"

namespace ola {
namespace acn {

class LLRPProbeRequestPDU : private PDU {
 public:
  static void PrependPDU(ola::io::IOStack *stack,
                         const ola::rdm::UID &lower_uid,
                         const ola::rdm::UID &upper_uid,
                         bool client_tcp_connection_inactive,
                         bool brokers_only,
                         const ola::rdm::UIDSet &known_uids);

  static const uint8_t VECTOR_PROBE_REQUEST_DATA = 0x01;

  static const unsigned int LLRP_KNOWN_UID_SIZE = 200;

  // bit masks for filter
  static const uint16_t FILTER_CLIENT_TCP_CONNECTION_INACTIVE = 0x0001;
  static const uint16_t FILTER_BROKERS_ONLY = 0x0002;

 private:
  PACK(
  struct llrp_probe_request_pdu_data_s {
    uint8_t lower_uid[ola::rdm::UID::LENGTH];
    uint8_t upper_uid[ola::rdm::UID::LENGTH];
    uint16_t filter;
    uint8_t known_uids[ola::rdm::UID::LENGTH * LLRP_KNOWN_UID_SIZE];
  });
  typedef struct llrp_probe_request_pdu_data_s llrp_probe_request_pdu_data;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_LLRPProbeRequestPDU_H_
