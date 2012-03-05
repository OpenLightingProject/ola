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
 * E131Layer.cpp
 * The E131Layer
 * Copyright (C) 2007 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include "ola/Logging.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/e131/e131/DMPE131Inflator.h"
#include "plugins/e131/e131/E131Inflator.h"
#include "plugins/e131/e131/E131Layer.h"
#include "plugins/e131/e131/E131PDU.h"
#include "plugins/e131/e131/RootSender.h"
#include "plugins/e131/e131/UDPTransport.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::IPV4Address;
using ola::network::HostToNetwork;

/*
 * Create a new E131Layer
 * @param root_layer the root layer to use
 */
E131Layer::E131Layer(ola::network::UdpSocket *socket,
                     RootSender *root_layer)
    : m_socket(socket),
      m_transport_impl(socket, &m_packer),
      m_root_sender(root_layer) {
  if (!m_root_sender)
    OLA_WARN << "root_layer is null, this won't work";
}


/*
 * Send a DMPPDU
 * @param header the E131Header
 * @param dmp_pdu the DMPPDU to send
 */
bool E131Layer::SendDMP(const E131Header &header, const DMPPDU *dmp_pdu) {
  if (!m_root_sender)
    return false;

  IPV4Address addr;
  if (!UniverseIP(header.Universe(), &addr)) {
    OLA_INFO << "could not convert universe to ip.";
    return false;
  }

  OutgoingUDPTransport transport(&m_transport_impl, addr);

  E131PDU pdu(DMPInflator::DMP_VECTOR, header, dmp_pdu);
  unsigned int vector = E131Inflator::E131_VECTOR;
  if (header.UsingRev2())
    vector = E131InflatorRev2::E131_REV2_VECTOR;
  return m_root_sender->SendPDU(vector, pdu, &transport);
}


/*
 * Calculate the IP that corresponds to a universe.
 * @param universe the universe id
 * @param addr where to store the address
 * @return true if this is a valid E1.31 universe, false otherwise
 */
bool E131Layer::UniverseIP(unsigned int universe, IPV4Address *addr) {
  *addr = IPV4Address(
      HostToNetwork(239U << 24 |
                    255U << 16 |
                    (universe & 0xFF00) |
                    (universe & 0xFF)));
  if (universe && (universe & 0xFFFF) != 0xFFFF)
    return true;

  OLA_WARN << "universe " << universe << " isn't a valid E1.31 universe";
  return false;
}
}  // e131
}  // plugin
}  // ola
