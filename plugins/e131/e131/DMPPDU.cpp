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


} // e131
} // ola
