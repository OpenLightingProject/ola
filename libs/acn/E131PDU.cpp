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
 * E131PDU.cpp
 * The E131PDU
 * Copyright (C) 2007 Simon Newton
 */

#include <string.h>
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/network/NetworkUtils.h"
#include "ola/strings/Utils.h"
#include "libs/acn/DMPPDU.h"
#include "libs/acn/E131PDU.h"

namespace ola {
namespace acn {

using ola::io::OutputStream;
using ola::network::HostToNetwork;

/*
 * Size of the header portion.
 */
unsigned int E131PDU::HeaderSize() const {
  if (m_header.UsingRev2())
    return sizeof(E131Rev2Header::e131_rev2_pdu_header);
  else
    return sizeof(E131Header::e131_pdu_header);
}


/*
 * Size of the data portion
 */
unsigned int E131PDU::DataSize() const {
  if (m_dmp_pdu)
    return m_dmp_pdu->Size();
  if (m_data)
    return m_data_size;
  return 0;
}


/*
 * Pack the header portion.
 */
bool E131PDU::PackHeader(uint8_t *data, unsigned int *length) const {
  unsigned int header_size = HeaderSize();

  if (*length < header_size) {
    OLA_WARN << "E131PDU::PackHeader: buffer too small, got " << *length
             << " required " << header_size;
    *length = 0;
    return false;
  }

  if (m_header.UsingRev2()) {
    E131Rev2Header::e131_rev2_pdu_header header;
    strings::CopyToFixedLengthBuffer(m_header.Source(), header.source,
                                     arraysize(header.source));
    header.priority = m_header.Priority();
    header.sequence = m_header.Sequence();
    header.universe = HostToNetwork(m_header.Universe());
    *length = sizeof(E131Rev2Header::e131_rev2_pdu_header);
    memcpy(data, &header, *length);
  } else {
    E131Header::e131_pdu_header header;
    strings::CopyToFixedLengthBuffer(m_header.Source(), header.source,
                                     arraysize(header.source));
    header.priority = m_header.Priority();
    header.reserved = 0;
    header.sequence = m_header.Sequence();
    header.options = static_cast<uint8_t>(
        (m_header.PreviewData() ? E131Header::PREVIEW_DATA_MASK : 0) |
        (m_header.StreamTerminated() ? E131Header::STREAM_TERMINATED_MASK : 0));
    header.universe = HostToNetwork(m_header.Universe());
    *length = sizeof(E131Header::e131_pdu_header);
    memcpy(data, &header, *length);
  }
  return true;
}


/*
 * Pack the data portion.
 */
bool E131PDU::PackData(uint8_t *data, unsigned int *length) const {
  if (m_dmp_pdu)
    return m_dmp_pdu->Pack(data, length);
  if (m_data) {
    memcpy(data, m_data, m_data_size);
    *length = m_data_size;
    return true;
  }
  *length = 0;
  return true;
}


/*
 * Pack the header into a buffer.
 */
void E131PDU::PackHeader(OutputStream *stream) const {
  if (m_header.UsingRev2()) {
    E131Rev2Header::e131_rev2_pdu_header header;
    strings::CopyToFixedLengthBuffer(m_header.Source(), header.source,
                                     arraysize(header.source));
    header.priority = m_header.Priority();
    header.sequence = m_header.Sequence();
    header.universe = HostToNetwork(m_header.Universe());
    stream->Write(reinterpret_cast<uint8_t*>(&header),
                  sizeof(E131Rev2Header::e131_rev2_pdu_header));
  } else {
    E131Header::e131_pdu_header header;
    strings::CopyToFixedLengthBuffer(m_header.Source(), header.source,
                                     arraysize(header.source));
    header.priority = m_header.Priority();
    header.reserved = 0;
    header.sequence = m_header.Sequence();
    header.options = static_cast<uint8_t>(
        (m_header.PreviewData() ? E131Header::PREVIEW_DATA_MASK : 0) |
        (m_header.StreamTerminated() ? E131Header::STREAM_TERMINATED_MASK : 0));
    header.universe = HostToNetwork(m_header.Universe());
    stream->Write(reinterpret_cast<uint8_t*>(&header),
                  sizeof(E131Header::e131_pdu_header));
  }
}


/*
 * Pack the data into a buffer
 */
void E131PDU::PackData(OutputStream *stream) const {
  if (m_dmp_pdu) {
    m_dmp_pdu->Write(stream);
  } else if (m_data) {
    stream->Write(m_data, m_data_size);
  }
}
}  // namespace acn
}  // namespace ola
