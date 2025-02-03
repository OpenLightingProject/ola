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
 * BrokerClientEntryRPTPDU.h
 * Interface for the BrokerClientEntryRPTPDU class
 * Copyright (C) 2023 Peter Newman
 */

#ifndef LIBS_ACN_BROKERCLIENTENTRYRPTPDU_H_
#define LIBS_ACN_BROKERCLIENTENTRYRPTPDU_H_

#include <ola/acn/CID.h>
#include <ola/io/IOStack.h>
#include <ola/rdm/UID.h>

#include "libs/acn/PDU.h"
#include "libs/acn/BrokerClientEntryHeader.h"

namespace ola {
namespace acn {

class BrokerClientEntryRPTPDU: public PDU {
 public:
    BrokerClientEntryRPTPDU(unsigned int vector,
                            const BrokerClientEntryHeader &header,
                            const ola::rdm::UID &client_uid,
                            uint8_t rpt_client_type,
                            const ola::acn::CID &binding_cid):
      PDU(vector, FOUR_BYTES, true),
      m_header(header),
      m_client_uid(client_uid),
      m_rpt_client_type(rpt_client_type),
      m_binding_cid(binding_cid) {}
    ~BrokerClientEntryRPTPDU() {}

    unsigned int HeaderSize() const;
    unsigned int DataSize() const;
    bool PackHeader(uint8_t *data, unsigned int *length) const;
    bool PackData(uint8_t *data, unsigned int *length) const;

    void PackHeader(ola::io::OutputStream *stream) const;
    void PackData(ola::io::OutputStream *stream) const;

    static void PrependPDU(ola::io::IOStack *stack,
                           uint32_t vector,
                           const ola::acn::CID &client_cid,
                           const ola::rdm::UID &client_uid,
                           uint8_t rpt_client_type,
                           const ola::acn::CID &binding_cid);

    PACK(
    struct broker_client_entry_rpt_pdu_data_s {
      uint8_t client_uid[ola::rdm::UID::LENGTH];
      uint8_t rpt_client_type;
      uint8_t binding_cid[ola::acn::CID::CID_LENGTH];
    });
    typedef struct broker_client_entry_rpt_pdu_data_s
        broker_client_entry_rpt_pdu_data;

 private:
    BrokerClientEntryHeader m_header;
    ola::rdm::UID m_client_uid;
    uint8_t m_rpt_client_type;
    ola::acn::CID m_binding_cid;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_BROKERCLIENTENTRYRPTPDU_H_
