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

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/e131/e131/DMPE131Inflator.h"
#include "plugins/e131/e131/E131Layer.h"
#include "plugins/e131/e131/E131Inflator.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::HostToNetwork;

/*
 * Create a new E131Layer
 * @param root_layer the root layer to use
 */
E131Layer::E131Layer(RootLayer *root_layer)
    : m_root_layer(root_layer) {
  m_root_layer->AddInflator(&m_e131_inflator);
  m_root_layer->AddInflator(&m_e131_rev2_inflator);
  if (!m_root_layer)
    OLA_WARN << "root_layer is null, this won't work";
}


/*
 * Send a DMPPDU
 * @param header the E131Header
 * @param dmp_pdu the DMPPDU to send
 */
bool E131Layer::SendDMP(const E131Header &header, const DMPPDU *dmp_pdu,
                        const CID *cid) {
  if (!m_root_layer)
    return false;

  struct in_addr addr;
  if (!UniverseIP(header.Universe(), addr)) {
    OLA_INFO << "could not convert universe to ip.";
    return false;
  }

  E131PDU pdu(DMPInflator::DMP_VECTOR, header, dmp_pdu);
  unsigned int vector = E131Inflator::E131_VECTOR;
  if (header.UsingRev2())
    vector = E131InflatorRev2::E131_REV2_VECTOR;

  if (cid)
    return m_root_layer->SendPDU(addr, vector, pdu, *cid);
  else
    return m_root_layer->SendPDU(addr, vector, pdu);
}


/*
 * Set the DMPInflator to use
 */
bool E131Layer::SetInflator(DMPE131Inflator *inflator) {
  bool ret = !m_e131_inflator.AddInflator(inflator);
  ret &= m_e131_rev2_inflator.AddInflator(inflator);
  return ret;
}


/*
 * Join a universe.
 */
bool E131Layer::JoinUniverse(unsigned int universe) {
  struct in_addr addr;

  if (!m_root_layer)
    return false;

  if (UniverseIP(universe, addr))
    return m_root_layer->JoinMulticast(addr);
  return false;
}


/*
 * Leave a universe
 */
bool E131Layer::LeaveUniverse(unsigned int universe) {
  struct in_addr addr;

  if (!m_root_layer)
    return false;

  if (UniverseIP(universe, addr))
    return m_root_layer->LeaveMulticast(addr);
  return false;
}


/*
 * Calculate the IP that corresponds to a universe.
 * @param universe the universe id
 * @param addr where to store the address
 * @return true if this is a valid E1.31 universe, false otherwise
 */
bool E131Layer::UniverseIP(unsigned int universe, struct in_addr &addr) {
  addr.s_addr = HostToNetwork(239 << 24 | 255 << 16 | (universe & 0xFF00) |
                      (universe & 0xFF));
  if (universe && (universe & 0xFFFF) != 0xFFFF)
    return true;

  OLA_WARN << "universe " << universe << " isn't a valid E1.31 universe";
  return false;
}
}  // e131
}  // plugin
}  // ola
