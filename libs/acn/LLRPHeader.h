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
 * LLRPHeader.h
 * The E1.33 LLRP Header
 * Copyright (C) 2020 Peter Newman
 */

#ifndef LIBS_ACN_LLRPHEADER_H_
#define LIBS_ACN_LLRPHEADER_H_

#include <ola/acn/CID.h>
#include <ola/base/Macro.h>

#include <stdint.h>

namespace ola {
namespace acn {

/*
 * Header for the LLRP layer
 */
class LLRPHeader {
 public:
    LLRPHeader()
        : m_transaction_number(0) {
    }

    LLRPHeader(const ola::acn::CID &destination_cid,
               uint32_t transaction_number)
        : m_destination_cid(destination_cid),
          m_transaction_number(transaction_number) {
    }
    ~LLRPHeader() {}

    const ola::acn::CID DestinationCid() const { return m_destination_cid; }
    uint32_t TransactionNumber() const { return m_transaction_number; }

    bool operator==(const LLRPHeader &other) const {
      return ((m_destination_cid == other.m_destination_cid) &&
        (m_transaction_number == other.m_transaction_number));
    }

    PACK(
    struct llrp_pdu_header_s {
      uint8_t destination_cid[CID::CID_LENGTH];
      uint32_t transaction_number;
    });
    typedef struct llrp_pdu_header_s llrp_pdu_header;

 private:
    ola::acn::CID m_destination_cid;
    uint32_t m_transaction_number;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_LLRPHEADER_H_
