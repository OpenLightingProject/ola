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
 * E131PDU.cpp
 * The E131PDU
 * Copyright (C) 2007-2009 Simon Newton
 */

#include <ola/Logging.h>
#include "E131PDU.h"

namespace ola {
namespace e131 {


/*
 * Size of the header portion.
 */
unsigned int E131PDU::HeaderSize() const {
  return sizeof(e131_pdu_header);
}


/*
 * Size of the data portion
 */
unsigned int E131PDU::DataSize() const {
  /*
  if (m_dmp)
    return m_dmp->Size()l
  */
  return 0;
}


/*
 * Pack the header portion.
 */
bool E131PDU::PackHeader(uint8_t *data, unsigned int &length) const {
  if (length < sizeof(e131_pdu_header)) {
    OLA_WARN << "E131PDU::PackHeader: buffer too small, got " << length <<
      " required " << sizeof(e131_pdu_header);
    length = 0;
    return false;
  }

  e131_pdu_header *header = (e131_pdu_header*) data;
  strncpy(header->source, m_header.Source().data(), SOURCE_NAME_LEN);
  header->sequence = m_header.Sequence();
  header->priority = m_header.Priority();
  header->universe = htons(m_header.Universe());
  length = sizeof(e131_pdu_header);
  return true;
}


/*
 * Pack the data portion.
 */
bool E131PDU::PackData(uint8_t *data, unsigned int &length) const {
  /*
  if (m_dmp)
    return m_dmp->Pack(data, length);
  */
  length = 0;
  return true;
}


} // e131
} // ola
