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
 * BrokerClientEntryPDU.h
 * Interface for the BrokerClientEntryPDU class
 * Copyright (C) 2023 Peter Newman
 */

#ifndef LIBS_ACN_BROKERCLIENTENTRYPDU_H_
#define LIBS_ACN_BROKERCLIENTENTRYPDU_H_

#include <ola/acn/CID.h>
#include <ola/io/IOStack.h>

#include "libs/acn/PDU.h"
#include "libs/acn/BrokerClientEntryHeader.h"

namespace ola {
namespace acn {

class BrokerClientEntryPDU: public PDU {
 public:
    BrokerClientEntryPDU(unsigned int vector,
            const BrokerClientEntryHeader &header,
            const PDU *pdu):
      PDU(vector, FOUR_BYTES, true),
      m_header(header),
      m_pdu(pdu) {}
    ~BrokerClientEntryPDU() {}

    unsigned int HeaderSize() const;
    unsigned int DataSize() const;
    bool PackHeader(uint8_t *data, unsigned int *length) const;
    bool PackData(uint8_t *data, unsigned int *length) const;

    void PackHeader(ola::io::OutputStream *stream) const;
    void PackData(ola::io::OutputStream *stream) const;

    static void PrependPDU(ola::io::IOStack *stack,
                           uint32_t vector,
                           const ola::acn::CID &client_cid);

 private:
    BrokerClientEntryHeader m_header;
    const PDU *m_pdu;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_BROKERCLIENTENTRYPDU_H_
