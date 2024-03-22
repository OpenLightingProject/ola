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

class LLRPProbeRequestPDU : public PDU {
 public:
  explicit LLRPProbeRequestPDU(unsigned int vector,
                               const ola::rdm::UID &lower_uid,
                               const ola::rdm::UID &upper_uid,
                               bool client_tcp_connection_inactive,
                               bool brokers_only,
                               const ola::rdm::UIDSet &known_uids):
    PDU(vector, ONE_BYTE, true),
    m_lower_uid(lower_uid),
    m_upper_uid(upper_uid),
    m_client_tcp_connection_inactive(client_tcp_connection_inactive),
    m_brokers_only(brokers_only),
    m_known_uids(known_uids) {}

  unsigned int HeaderSize() const { return 0; }
  bool PackHeader(OLA_UNUSED uint8_t *data,
                  unsigned int *length) const {
    *length = 0;
    return true;
  }
  void PackHeader(OLA_UNUSED ola::io::OutputStream *stream) const {}

  unsigned int DataSize() const;
  bool PackData(uint8_t *data, unsigned int *length) const;
  void PackData(ola::io::OutputStream *stream) const;

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

  PACK(
  struct llrp_probe_request_pdu_data_s {
    uint8_t lower_uid[ola::rdm::UID::LENGTH];
    uint8_t upper_uid[ola::rdm::UID::LENGTH];
    uint16_t filter;
    uint8_t known_uids[ola::rdm::UID::LENGTH * LLRP_KNOWN_UID_SIZE];
  });
  typedef struct llrp_probe_request_pdu_data_s llrp_probe_request_pdu_data;

 private:
  const ola::rdm::UID m_lower_uid;
  const ola::rdm::UID m_upper_uid;
  bool m_client_tcp_connection_inactive;
  bool m_brokers_only;
  const ola::rdm::UIDSet m_known_uids;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_LLRPPROBEREQUESTPDU_H_
