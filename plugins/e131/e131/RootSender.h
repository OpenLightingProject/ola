/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * RootSender.h
 * The RootSender class manages the sending of Root Layer PDUs.
 * Copyright (C) 2007 Simon Newton
 */

#ifndef PLUGINS_E131_E131_ROOTSENDER_H_
#define PLUGINS_E131_E131_ROOTSENDER_H_

#include "plugins/e131/e131/CID.h"
#include "plugins/e131/e131/PDU.h"
#include "plugins/e131/e131/RootPDU.h"
#include "plugins/e131/e131/Transport.h"

namespace ola {
namespace plugin {
namespace e131 {


class RootSender {
  public:
    explicit RootSender(const CID &cid);
    ~RootSender() {}

    // Convenience method to encapsulate & send a single PDU
    bool SendPDU(unsigned int vector,
                 const PDU &pdu,
                 OutgoingTransport *transport);
    // Use for testing to force a message from a particular cid
    bool SendPDU(unsigned int vector,
                 const PDU &pdu,
                 const CID &cid,
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

    RootSender(const RootSender&);
    RootSender& operator=(const RootSender&);
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_ROOTSENDER_H_
