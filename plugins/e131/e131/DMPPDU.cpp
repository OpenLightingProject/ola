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
 * DMPPDU.cpp
 * The DMPPDU
 * Copyright (C) 2007-2009 Simon Newton
 */

#include <ola/Logging.h>
#include "DMPPDU.h"

namespace ola {
namespace e131 {


/*
 * Pack the header portion.
 */
bool DMPPDU::PackHeader(uint8_t *data, unsigned int &length) const {
  if (length < DMPHeader::DMP_HEADER_SIZE) {
    OLA_WARN << "DMPPDU::PackHeader: buffer too small, got " << length <<
      " required " << DMPHeader::DMP_HEADER_SIZE;
    length = 0;
    return false;
  }
  *data = m_header.Header();
  length = DMPHeader::DMP_HEADER_SIZE;
  return true;
}


/*
 * Create a new Single Address GetProperty PDU.
 * @param is_virtual set to true if this is a virtual address
 * @param is_relative set to true if this is a relative address
 * @param start the start offset
 * @return A pointer to a DMPGetProperty.
 */
const DMPPDU *NewDMPGetProperty(bool is_virtual,
                                bool is_relative,
                                unsigned int start) {
  if (start > MAX_TWO_BYTE)
    return _CreateDMPGetProperty<uint32_t>(is_virtual, is_relative, start);
  else if (start > MAX_ONE_BYTE)
    return _CreateDMPGetProperty<uint16_t>(is_virtual, is_relative, start);
  return _CreateDMPGetProperty<uint8_t>(is_virtual, is_relative, start);
}


/*
 * Create a new range address GetProperty PDU.
 * @param is_virtual set to true if this is a virtual address
 * @param is_relative set to true if this is a relative address
 * @param start the start offset
 * @param increment the increments between addresses
 * @param number the number of addresses defined
 * @return A pointer to a DMPGetProperty.
 */
const DMPPDU *NewRangeDMPGetProperty(
    bool is_virtual,
    bool is_relative,
    unsigned int start,
    unsigned int increment,
    unsigned int number) {

  if (start > MAX_TWO_BYTE || increment > MAX_TWO_BYTE ||
      number > MAX_TWO_BYTE)
    return _CreateRangeDMPGetProperty<uint32_t>(is_virtual,
                                                is_relative,
                                                start,
                                                increment,
                                                number);
  else if (start > MAX_ONE_BYTE || increment > MAX_ONE_BYTE ||
             number > MAX_ONE_BYTE)
    return _CreateRangeDMPGetProperty<uint16_t>(is_virtual,
                                                is_relative,
                                                start,
                                                increment,
                                                number);
  return _CreateRangeDMPGetProperty<uint8_t>(is_virtual,
                                             is_relative,
                                             start,
                                             increment,
                                             number);
}


} // e131
} // ola
