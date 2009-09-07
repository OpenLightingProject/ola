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
 * Pack the header into a buffer.
 */
bool RootPDU::PackHeader(uint8_t *data, unsigned int &length) const {
  if (length < HeaderSize()) {
    length = 0;
    return false;
  }

  m_cid.Pack(data);
  length = HeaderSize();
  return true;
}


/*
 * Pack the data into a buffer
 */
bool RootPDU::PackData(uint8_t *data, unsigned int &length) const {
  if (m_block)
    return m_block->Pack(data, length);

  length = 0;
  return true;
}


void RootPDU::SetBlock(const PDUBlock<PDU> *block) {
  m_block = block;
  m_block_size = m_block ? block->Size() : 0;
}

} // e131
} // ola
