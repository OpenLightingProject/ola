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
 * RootLayer.h
 * Interface for the RootLayer class, this abstracts the encapsulation and
 * sending of PDUs contained within RootPDUs as well as the setting of
 * inflators.
 * Copyright (C) 2007 Simon Newton
 */

#ifndef PLUGINS_E131_E131_ROOTLAYER_H_
#define PLUGINS_E131_E131_ROOTLAYER_H_

#include "ola/network/IPV4Address.h"
#include "plugins/e131/e131/CID.h"
#include "plugins/e131/e131/PDU.h"
#include "plugins/e131/e131/RootPDU.h"
#include "plugins/e131/e131/RootInflator.h"
#include "plugins/e131/e131/UDPTransport.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::IPV4Address;
using ola::plugin::e131::UDPTransport;

class RootLayer {
  public:
    RootLayer(UDPTransport *transport, const CID &cid);
    ~RootLayer() {}

    bool AddInflator(BaseInflator *inflator);

    // Convenience method to encapsulate & send a single PDU
    bool SendPDU(unsigned int vector,
                 const PDU &pdu,
                 const IPV4Address &destination,
                 uint16_t port = UDPTransport::ACN_PORT);
    // Use for testing to force a message from a particular cid
    bool SendPDU(unsigned int vector,
                 const PDU &pdu,
                 const CID &cid,
                 const IPV4Address &destination,
                 uint16_t port = UDPTransport::ACN_PORT);
    // Encapsulation & send a block of PDUs
    bool SendPDUBlock(unsigned int vector,
                      const PDUBlock<PDU> &block,
                      const IPV4Address &destination,
                      uint16_t port = UDPTransport::ACN_PORT);

    // TODO(simon): add methods to queue and send PDUs/blocks with different
    // vectors

    bool JoinMulticast(const IPV4Address &group);
    bool LeaveMulticast(const IPV4Address &group);

  private:
    UDPTransport *m_transport;
    RootInflator m_root_inflator;
    PDUBlock<PDU> m_working_block;
    PDUBlock<PDU> m_root_block;
    RootPDU m_root_pdu;

    RootLayer(const RootLayer&);
    RootLayer& operator=(const RootLayer&);
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_ROOTLAYER_H_
