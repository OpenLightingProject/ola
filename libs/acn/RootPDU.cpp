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
 * RootPDU.cpp
 * The RootPDU class
 * Copyright (C) 2007 Simon Newton
 */

#include "ola/Logging.h"
#include "ola/io/IOStack.h"
#include "libs/acn/BaseInflator.h"
#include "libs/acn/RootPDU.h"

namespace ola {
namespace acn {

using ola::acn::CID;
using ola::io::IOStack;
using ola::io::OutputStream;
using ola::network::HostToNetwork;

/*
 * Pack the header into a buffer.
 */
bool RootPDU::PackHeader(uint8_t *data, unsigned int *length) const {
  if (*length < HeaderSize()) {
    *length = 0;
    return false;
  }

  m_cid.Pack(data);
  *length = HeaderSize();
  return true;
}


/*
 * Pack the data into a buffer
 */
bool RootPDU::PackData(uint8_t *data, unsigned int *length) const {
  if (m_block)
    return m_block->Pack(data, length);

  *length = 0;
  return true;
}


/*
 * Pack the header into a buffer.
 */
void RootPDU::PackHeader(OutputStream *stream) const {
  uint8_t cid[CID::CID_LENGTH];
  m_cid.Pack(cid);
  stream->Write(cid, CID::CID_LENGTH);
}


/*
 * Pack the data into a buffer
 */
void RootPDU::PackData(OutputStream *stream) const {
  if (m_block)
    m_block->Write(stream);
}


void RootPDU::SetBlock(const PDUBlock<PDU> *block) {
  m_block = block;
  m_block_size = m_block ? block->Size() : 0;
}


/*
 * Prepend a Root Layer flags, length, vector & header
 */
void RootPDU::PrependPDU(IOStack *stack, uint32_t vector, const CID &cid) {
  cid.Write(stack);

  vector = HostToNetwork(vector);
  stack->Write(reinterpret_cast<uint8_t*>(&vector), sizeof(vector));
  PrependFlagsAndLength(stack);
}
}  // namespace acn
}  // namespace ola
