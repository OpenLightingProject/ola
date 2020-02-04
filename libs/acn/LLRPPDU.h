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
 * LLRPPDU.h
 * Interface for the LLRPPDU class
 * Copyright (C) 2020 Peter Newman
 */

#ifndef LIBS_ACN_LLRPPDU_H_
#define LIBS_ACN_LLRPPDU_H_

#include <ola/acn/CID.h>
#include <ola/io/IOStack.h>

#include "libs/acn/PDU.h"
#include "libs/acn/LLRPHeader.h"

namespace ola {
namespace acn {

class LLRPPDU: public PDU {
 public:
    LLRPPDU(unsigned int vector,
            const LLRPHeader &header,
            const PDU *pdu):
      PDU(vector),
      m_header(header),
      m_pdu(pdu) {}
    ~LLRPPDU() {}

    unsigned int HeaderSize() const;
    unsigned int DataSize() const;
    bool PackHeader(uint8_t *data, unsigned int *length) const;
    bool PackData(uint8_t *data, unsigned int *length) const;

    void PackHeader(ola::io::OutputStream *stream) const;
    void PackData(ola::io::OutputStream *stream) const;

    static void PrependPDU(ola::io::IOStack *stack, uint32_t vector,
                           const ola::acn::CID &destination_cid, uint32_t transaction_number);

 private:
    LLRPHeader m_header;
    const PDU *m_pdu;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_LLRPPDU_H_
