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
 * The Inflator for the E1.31 PDUs
 * Copyright (C) 2007-2009 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <string>
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/e131/e131/E131Inflator.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::NetworkToHost;

/*
 * Decode the E1.31 headers. If data is null we're expected to use the last
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
      E131Header::e131_pdu_header raw_header;
      memcpy(&raw_header, data, sizeof(E131Header::e131_pdu_header));
      raw_header.source[E131Header::SOURCE_NAME_LEN - 1] = 0x00;
      E131Header header(
          raw_header.source,
          raw_header.priority,
          raw_header.sequence,
          NetworkToHost(raw_header.universe),
          raw_header.options & E131Header::PREVIEW_DATA_MASK,
          raw_header.options & E131Header::STREAM_TERMINATED_MASK,
          raw_header.options & E131Header::RDM_MANAGEMENT_MASK);
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
 * Decode the E1.31 headers. If data is null we're expected to use the last
 * header we got.
 * @param headers the HeaderSet to add to
 * @param data a pointer to the data
 * @param length length of the data
 * @returns true if successful, false otherwise
 */
bool E131InflatorRev2::DecodeHeader(HeaderSet &headers,
                                    const uint8_t *data,
                                    unsigned int length,
                                    unsigned int &bytes_used) {
  if (data) {
    // the header bit was set, decode it
    if (length >= sizeof(E131Rev2Header::e131_rev2_pdu_header)) {
      E131Rev2Header::e131_rev2_pdu_header raw_header;
      memcpy(&raw_header, data, sizeof(E131Rev2Header::e131_rev2_pdu_header));
      raw_header.source[E131Rev2Header::REV2_SOURCE_NAME_LEN - 1] = 0x00;
      E131Rev2Header header(raw_header.source,
                            raw_header.priority,
                            raw_header.sequence,
                            NetworkToHost(raw_header.universe));
      m_last_header = header;
      m_last_header_valid = true;
      headers.SetE131Header(header);
      bytes_used = sizeof(E131Rev2Header::e131_rev2_pdu_header);
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
}  // e131
}  // plugin
}  // ola
