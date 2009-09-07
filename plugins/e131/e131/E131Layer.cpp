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

#include <ola/Logging.h>
#include "E131Layer.h"

namespace ola {
namespace e131 {

/*
 * Create a new E131Layer
 * @param root_layer the root layer to use
 */
E131Layer::E131Layer(RootLayer *root_layer):
  m_root_layer(root_layer) {

  m_root_layer->AddInflator(&m_e131_inflator);
}


/*
 * Add an inflator to the root level
bool E131Layer::AddInflator(BaseInflator *inflator) {
  return m_root_inflator.AddInflator(inflator);
}
 */


/*
 * Encapsulate this PDUBlock in a E131PDU and send it to the destination.
 * @param addr where to send the PDU
 * @param vector the vector to use at the root level
 * @param block the PDUBlock to send.
bool E131Layer::SendPDUBlock(struct in_addr &addr,
                            unsigned int vector,
                            const PDUBlock<PDU> &block) {


  if (!m_root_layer) {
    OLA_WARN << "root_layer is null";
    return false;
  }

  E131PDU pdu(0, header, dmp_pdu);
  return m_root_layer->SendPDU(  , E131Inflator::E131_VECTOR, pdu);
}
  */

} //e131
} //ola
