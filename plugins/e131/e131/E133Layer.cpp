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
 * E133Layer.cpp
 * The E133Layer
 * Copyright (C) 2011 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/e131/e131/DMPE133Inflator.h"
#include "plugins/e131/e131/E133Layer.h"
#include "plugins/e131/e131/E133Inflator.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::HostToNetwork;

/*
 * Create a new E133Layer
 * @param root_layer the root layer to use
 */
E133Layer::E133Layer(RootLayer *root_layer)
    : m_root_layer(root_layer) {
  m_root_layer->AddInflator(&m_e133_inflator);
  if (!m_root_layer)
    OLA_WARN << "root_layer is null, this won't work";
}


/*
 * Send a DMPPDU
 * @param header the E133Header
 * @param dmp_pdu the DMPPDU to send
 */
bool E133Layer::SendDMP(const ola::network::IPV4Address &destination,
                        const E133Header &header,
                        const DMPPDU *dmp_pdu) {
  if (!m_root_layer)
    return false;

  E133PDU pdu(DMPInflator::DMP_VECTOR, header, dmp_pdu);
  unsigned int vector = E133Inflator::E133_VECTOR;
  return m_root_layer->SendPDU(destination, vector, pdu);
}


/*
 * Set the DMPInflator to use
 */
bool E133Layer::SetInflator(DMPE133Inflator *inflator) {
  return m_e133_inflator.AddInflator(inflator);
}
}  // e131
}  // plugin
}  // ola
