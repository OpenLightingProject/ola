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
 * E131Inflator.cpp
 * The Inflator for the root level packets over UDP
 * Copyright (C) 2007 Simon Newton
 */

#include <ola/Logging.h>
#include "E131Inflator.h"

namespace ola {
namespace e131 {


/*
 * Decode the root headers. If data is null we're expected to use the last
 * header we got.
 * @param headers the HeaderSet to add to
 * @param data a pointer to the data
 * @param length length of the data
 * @returns true if successful, false otherwise
 */
bool E131Inflator::DecodeHeader(HeaderSet &headers,
                                const uint8_t *data,
                                unsigned int length,
                                unsigned int &bytes_used) {

  if (data) {
    // the header bit was set, decode it
    if (length >= sizeof(E131Header::e131_pdu_header)) {
      E131Header::e131_pdu_header *raw_header =
        (E131Header::e131_pdu_header *) data;
      raw_header->source[E131Header::SOURCE_NAME_LEN - 1] = 0x00;
      string source_name(raw_header->source);
      E131Header header(source_name,
                        raw_header->priority,
                        raw_header->sequence,
                        ntohs(raw_header->universe));
      m_last_header = header;
      m_last_header_valid = true;
      headers.SetE131Header(header);
      bytes_used = sizeof(E131Header::e131_pdu_header);
      return true;
    }
    bytes_used = 0;
    return false;
  }

  // use the last header if it exists
  bytes_used = 0;
  if (!m_last_header_valid) {
    OLA_WARN << "Missing E131 Header data";
    return false;
  }
  headers.SetE131Header(m_last_header);
  return true;
}


/*
 * Reset the header field
 */
void E131Inflator::ResetHeaderField() {
  m_last_header_valid = false;
}

} // e131
} // ola
