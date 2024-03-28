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
 * LLRPProbeReplyPDU.h
 * The LLRPProbeReplyPDU class
 * Copyright (C) 2020 Peter Newman
 */

#ifndef LIBS_ACN_LLRPPROBEREPLYPDU_H_
#define LIBS_ACN_LLRPPROBEREPLYPDU_H_

#include <ola/io/IOStack.h>
#include <ola/network/MACAddress.h>
#include <ola/rdm/UID.h>

#include "libs/acn/PDU.h"

namespace ola {
namespace acn {

class LLRPProbeReplyPDU : public PDU {
 public:
  typedef enum {
    LLRP_COMPONENT_TYPE_RPT_DEVICE = 0,  /**< Device */
    LLRP_COMPONENT_TYPE_RPT_CONTROLLER = 1,  /**< Controller */
    LLRP_COMPONENT_TYPE_BROKER = 2,  /**< Broker */
    LLRP_COMPONENT_TYPE_NON_RDMNET = 0xff,  /**< Non-RDMnet */
  } LLRPComponentType;

  explicit LLRPProbeReplyPDU(unsigned int vector,
                             const ola::rdm::UID &target_uid,
                             const ola::network::MACAddress &hardware_address,
                             const LLRPComponentType type):
    PDU(vector, ONE_BYTE, true),
    m_target_uid(target_uid),
    m_hardware_address(hardware_address),
    m_type(type) {}

  unsigned int HeaderSize() const { return 0; }
  bool PackHeader(OLA_UNUSED uint8_t *data,
                  unsigned int *length) const {
    *length = 0;
    return true;
  }
  void PackHeader(OLA_UNUSED ola::io::OutputStream *stream) const {}

  unsigned int DataSize() const { return sizeof(llrp_probe_reply_pdu_data); }
  bool PackData(uint8_t *data, unsigned int *length) const;
  void PackData(ola::io::OutputStream *stream) const;

  static void PrependPDU(ola::io::IOStack *stack,
                         const ola::rdm::UID &target_uid,
                         const ola::network::MACAddress &hardware_address,
                         const LLRPComponentType type);

  static const uint8_t VECTOR_PROBE_REPLY_DATA = 0x01;

  PACK(
  struct llrp_probe_reply_pdu_data_s {
    uint8_t target_uid[ola::rdm::UID::LENGTH];
    uint8_t hardware_address[ola::network::MACAddress::LENGTH];
    uint8_t type;
  });
  typedef struct llrp_probe_reply_pdu_data_s llrp_probe_reply_pdu_data;

 private:
  const ola::rdm::UID m_target_uid;
  const ola::network::MACAddress m_hardware_address;
  const LLRPComponentType m_type;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_LLRPPROBEREPLYPDU_H_
