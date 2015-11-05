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
 * E131Sender.cpp
 * The E131Sender
 * Copyright (C) 2007 Simon Newton
 */

#include "ola/Logging.h"
#include "ola/acn/ACNVectors.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "ola/util/Utils.h"
#include "libs/acn/DMPE131Inflator.h"
#include "libs/acn/E131Inflator.h"
#include "libs/acn/E131Sender.h"
#include "libs/acn/E131PDU.h"
#include "libs/acn/RootSender.h"
#include "libs/acn/UDPTransport.h"

namespace ola {
namespace acn {

using ola::network::IPV4Address;
using ola::network::HostToNetwork;

/*
 * Create a new E131Sender
 * @param root_sender the root layer to use
 */
E131Sender::E131Sender(ola::network::UDPSocket *socket,
                       RootSender *root_sender)
    : m_socket(socket),
      m_transport_impl(socket, &m_packer),
      m_root_sender(root_sender) {
  if (!m_root_sender) {
    OLA_WARN << "root_sender is null, this won't work";
  }
}


/*
 * Send a DMPPDU
 * @param header the E131Header
 * @param dmp_pdu the DMPPDU to send
 */
bool E131Sender::SendDMP(const E131Header &header, const DMPPDU *dmp_pdu) {
  if (!m_root_sender) {
    return false;
  }

  IPV4Address addr;
  if (!UniverseIP(header.Universe(), &addr)) {
    OLA_INFO << "Could not convert universe " << header.Universe()
             << " to IP.";
    return false;
  }

  OutgoingUDPTransport transport(&m_transport_impl, addr);

  E131PDU pdu(ola::acn::VECTOR_E131_DATA, header, dmp_pdu);
  unsigned int vector = ola::acn::VECTOR_ROOT_E131;
  if (header.UsingRev2()) {
    vector = ola::acn::VECTOR_ROOT_E131_REV2;
  }
  return m_root_sender->SendPDU(vector, pdu, &transport);
}

bool E131Sender::SendDiscoveryData(const E131Header &header,
                                   const uint8_t *data,
                                   unsigned int data_size) {
  if (!m_root_sender) {
    return false;
  }

  IPV4Address addr;
  if (!UniverseIP(header.Universe(), &addr)) {
    OLA_INFO << "Could not convert universe " << header.Universe()
             << " to IP.";
    return false;
  }

  OutgoingUDPTransport transport(&m_transport_impl, addr);

  E131PDU pdu(ola::acn::VECTOR_E131_DISCOVERY, header, data, data_size);
  unsigned int vector = ola::acn::VECTOR_ROOT_E131;
  return m_root_sender->SendPDU(vector, pdu, &transport);
}


/*
 * Calculate the IP that corresponds to a universe.
 * @param universe the universe id
 * @param addr where to store the address
 * @return true if this is a valid E1.31 universe, false otherwise
 */
bool E131Sender::UniverseIP(uint16_t universe, IPV4Address *addr) {
  uint8_t universe_high;
  uint8_t universe_low;
  ola::utils::SplitUInt16(universe, &universe_high, &universe_low);
  *addr = IPV4Address(
      HostToNetwork(ola::utils::JoinUInt8(239,
                                          255,
                                          universe_high,
                                          universe_low)));
  if (universe && (universe != 0xFFFF)) {
    return true;
  }

  OLA_WARN << "Universe " << universe << " isn't a valid E1.31 universe";
  return false;
}
}  // namespace acn
}  // namespace ola
