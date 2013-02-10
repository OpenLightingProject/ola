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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * DMPInflator.cpp
 * The Inflator for the DMP PDUs
 * Copyright (C) 2007-2009 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include "ola/Logging.h"
#include "plugins/e131/e131/DMPInflator.h"

namespace ola {
namespace plugin {
namespace e131 {


/*
 * Decode the DMP header. If data is null we're expected to use the last
 * header we got.
 * @param headers the HeaderSet to add to
 * @param data a pointer to the data
 * @param length length of the data
 * @returns true if successful, false otherwise
 */
bool DMPInflator::DecodeHeader(HeaderSet &headers,
                               const uint8_t *data,
                               unsigned int length,
                               unsigned int &bytes_used) {
  if (data) {
    // the header bit was set, decode it
    if (length >= DMPHeader::DMP_HEADER_SIZE) {
      DMPHeader header(*data);
      m_last_header = header;
      m_last_header_valid = true;
      headers.SetDMPHeader(header);
      bytes_used = DMPHeader::DMP_HEADER_SIZE;
      return true;
    }
    bytes_used = 0;
    return false;
  }

  // use the last header if it exists
  bytes_used = 0;
  if (!m_last_header_valid) {
    OLA_WARN << "Missing DMP Header data";
    return false;
  }
  headers.SetDMPHeader(m_last_header);
  return true;
}


/*
 * Reset the header field
 */
void DMPInflator::ResetHeaderField() {
  m_last_header_valid = false;
}
}  // e131
}  // plugin
}  // ola
