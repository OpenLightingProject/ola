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
 * RPTInflator.cpp
 * The Inflator for E1.33 RPT
 * Copyright (C) 2023 Peter Newman
 */

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "libs/acn/RPTInflator.h"

namespace ola {
namespace acn {

using ola::network::NetworkToHost;

/*
 * Decode the E1.33 RPT headers. If data is null we're expected to use the
 * last header we got.
 * @param headers the HeaderSet to add to
 * @param data a pointer to the data
 * @param length length of the data
 * @returns true if successful, false otherwise
 */
bool RPTInflator::DecodeHeader(HeaderSet *headers,
                                const uint8_t *data,
                                unsigned int length,
                                unsigned int *bytes_used) {
  if (data) {
    // the header bit was set, decode it
    if (length >= sizeof(RPTHeader::rpt_pdu_header)) {
      RPTHeader::rpt_pdu_header raw_header;
      memcpy(&raw_header, data, sizeof(RPTHeader::rpt_pdu_header));
      RPTHeader header(
          ola::rdm::UID(raw_header.source_uid),
          NetworkToHost(raw_header.source_endpoint),
          ola::rdm::UID(raw_header.destination_uid),
          NetworkToHost(raw_header.destination_endpoint),
          NetworkToHost(raw_header.sequence));
      m_last_header = header;
      m_last_header_valid = true;
      headers->SetRPTHeader(header);
      *bytes_used = sizeof(RPTHeader::rpt_pdu_header);
      return true;
    }
    *bytes_used = 0;
    return false;
  }

  // use the last header if it exists
  *bytes_used = 0;
  if (!m_last_header_valid) {
    OLA_WARN << "Missing E1.33 RPT Header data";
    return false;
  }
  headers->SetRPTHeader(m_last_header);
  return true;
}
}  // namespace acn
}  // namespace ola
