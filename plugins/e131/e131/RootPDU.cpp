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
 * RootPDU.cpp
 * The RootPDU class
 * Copyright (C) 2007-2009 Simon Newton
 */

#include <ola/Logging.h>
#include "RootPDU.h"
#include "BaseInflator.h"

namespace ola {
namespace e131 {

/*
 * Return the length of this PDU
 * @return length of the pdu
 */
unsigned int RootPDU::Size() const {
  unsigned int block_size = m_block ? m_block->Size() : 0;
  unsigned int length = sizeof(m_vector) + CID::CID_LENGTH + block_size;

  if (length > TWOB_LENGTH_LIMIT - 2)
    length += 1;
  length += 2;
  return length;
}


/*
 * Pack this PDU into a buffer
 * @param buffer pointer to the buffer
 * @param length length of the buffer
 * @return false on error, true otherwise
 */
bool RootPDU::Pack(uint8_t *buffer, unsigned int &length) const {
  unsigned int size = Size();
  int offset = 0;

  if (length < size) {
    OLA_WARN << "RootPDU Pack: buffer too small, required " << size << ", got "
      << length;
    length = 0;
    return false;
  }

  if (size <= TWOB_LENGTH_LIMIT) {
    buffer[0] = (size & 0x0f00) >> 8;
    buffer[1] = size & 0xff;
  } else {
    buffer[0] = (size & 0x0f0000) >> 16;
    buffer[1] = (size & 0xff00) >> 8;
    buffer[2] = size & 0xff;
    offset += 1;
  }

  buffer[0] |= BaseInflator::VFLAG_MASK;
  buffer[0] |= BaseInflator::HFLAG_MASK;
  buffer[0] |= BaseInflator::DFLAG_MASK;
  offset += 2;

  uint32_t *vector = (uint32_t*) (buffer + offset);
  *vector = htonl(m_vector);
  offset += 4;

  m_cid.Pack(buffer + offset);
  offset += CID::CID_LENGTH;

  bool status = true;
  if (m_block) {
    unsigned int remaining = length - offset;
    status = m_block->Pack(buffer + offset, remaining);
    offset += remaining;
  }

  length = offset;
  return status;
}

} // e131
} // ola
