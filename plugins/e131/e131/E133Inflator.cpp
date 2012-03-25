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
 * E133Inflator.cpp
 * The Inflator for the E1.33 PDUs
 * Copyright (C) 2011 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <string>
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/e131/e131/E133Inflator.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::NetworkToHost;

/*
 * Decode the E1.33 headers. If data is null we're expected to use the last
 * header we got.
 * @param headers the HeaderSet to add to
 * @param data a pointer to the data
 * @param length length of the data
 * @returns true if successful, false otherwise
 */
bool E133Inflator::DecodeHeader(HeaderSet &headers,
                                const uint8_t *data,
                                unsigned int length,
                                unsigned int &bytes_used) {
  if (data) {
    // the header bit was set, decode it
    if (length >= sizeof(E133Header::e133_pdu_header)) {
      E133Header::e133_pdu_header raw_header;
      memcpy(&raw_header, data, sizeof(E133Header::e133_pdu_header));
      raw_header.source[E133Header::SOURCE_NAME_LEN - 1] = 0x00;
      E133Header header(
          raw_header.source,
          NetworkToHost(raw_header.sequence),
          NetworkToHost(raw_header.endpoint),
          raw_header.options & E133Header::E133_RX_ACK_MASK,
          raw_header.options & E133Header::E133_TIMEOUT_MASK);
      m_last_header = header;
      m_last_header_valid = true;
      headers.SetE133Header(header);
      bytes_used = sizeof(E133Header::e133_pdu_header);
      return true;
    }
    bytes_used = 0;
    return false;
  }

  // use the last header if it exists
  bytes_used = 0;
  if (!m_last_header_valid) {
    OLA_WARN << "Missing E1.33 Header data";
    return false;
  }
  headers.SetE133Header(m_last_header);
  return true;
}
}  // e131
}  // plugin
}  // ola
