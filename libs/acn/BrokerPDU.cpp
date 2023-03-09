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
 * BrokerPDU.cpp
 * The BrokerPDU
 * Copyright (C) 2023 Peter Newman
 */


#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/network/NetworkUtils.h"
#include "libs/acn/BrokerPDU.h"

namespace ola {
namespace acn {

using ola::io::OutputStream;
using ola::network::HostToNetwork;

/*
 * Size of the data portion
 */
unsigned int BrokerPDU::DataSize() const {
  return m_pdu ? m_pdu->Size() : 0;
}


/*
 * Pack the data portion.
 */
bool BrokerPDU::PackData(uint8_t *data, unsigned int *length) const {
  if (m_pdu)
    return m_pdu->Pack(data, length);
  *length = 0;
  return true;
}


/*
 * Pack the data into a buffer
 */
void BrokerPDU::PackData(OutputStream *stream) const {
  if (m_pdu)
    m_pdu->Write(stream);
}


void BrokerPDU::PrependPDU(ola::io::IOStack *stack,
                           uint16_t vector) {
  vector = HostToNetwork(vector);
  stack->Write(reinterpret_cast<uint8_t*>(&vector), sizeof(vector));
  PrependFlagsAndLength(stack, VFLAG_MASK | HFLAG_MASK | DFLAG_MASK, true);
}
}  // namespace acn
}  // namespace ola
