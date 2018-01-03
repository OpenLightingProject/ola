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
 * RootSender.h
 * The RootSender class manages the sending of Root Layer PDUs.
 * Copyright (C) 2007 Simon Newton
 */

#ifndef LIBS_ACN_ROOTSENDER_H_
#define LIBS_ACN_ROOTSENDER_H_

#include "ola/acn/CID.h"
#include "ola/base/Macro.h"
#include "libs/acn/PDU.h"
#include "libs/acn/RootPDU.h"
#include "libs/acn/Transport.h"

namespace ola {
namespace acn {

class RootSender {
 public:
    explicit RootSender(const ola::acn::CID &cid);
    ~RootSender() {}

    // Convenience method to encapsulate & send a single PDU
    bool SendPDU(unsigned int vector,
                 const PDU &pdu,
                 OutgoingTransport *transport);
    // Send a RootPDU with no data
    bool SendEmpty(unsigned int vector,
                   OutgoingTransport *transport);
    // Use for testing to force a message from a particular cid
    bool SendPDU(unsigned int vector,
                 const PDU &pdu,
                 const ola::acn::CID &cid,
                 OutgoingTransport *transport);
    // Encapsulation & send a block of PDUs
    bool SendPDUBlock(unsigned int vector,
                      const PDUBlock<PDU> &block,
                      OutgoingTransport *transport);

    // TODO(simon): add methods to queue and send PDUs/blocks with different
    // vectors

 private:
    PDUBlock<PDU> m_working_block;
    PDUBlock<PDU> m_root_block;
    RootPDU m_root_pdu;

    DISALLOW_COPY_AND_ASSIGN(RootSender);
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_ROOTSENDER_H_
