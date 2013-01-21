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
 * E133PDU.cpp
 * The E133PDU
 * Copyright (C) 2011 Simon Newton
 */


#include <string.h>
#include <ola/Logging.h>
#include <ola/network/NetworkUtils.h>
#include "plugins/e131/e131/E133PDU.h"
#include "plugins/e131/e131/RDMPDU.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::HostToNetwork;

/*
 * Size of the header portion.
 */
unsigned int E133PDU::HeaderSize() const {
  return sizeof(E133Header::e133_pdu_header);
}


/*
 * Size of the data portion
 */
unsigned int E133PDU::DataSize() const {
  return m_pdu ? m_pdu->Size() : 0;
}


/*
 * Pack the header portion.
 */
bool E133PDU::PackHeader(uint8_t *data, unsigned int &length) const {
  unsigned int header_size = HeaderSize();

  if (length < header_size) {
    OLA_WARN << "E133PDU::PackHeader: buffer too small, got " << length <<
      " required " << header_size;
    length = 0;
    return false;
  }

  E133Header::e133_pdu_header header;
  strncpy(header.source, m_header.Source().data(),
          E133Header::SOURCE_NAME_LEN);
  header.sequence = HostToNetwork(m_header.Sequence());
  header.endpoint = HostToNetwork(m_header.Endpoint());
  header.options = static_cast<uint8_t>(
      m_header.RxAcknowledge() ? E133Header::E133_RX_ACK_MASK : 0);
  length = sizeof(E133Header::e133_pdu_header);
  memcpy(data, &header, length);
  return true;
}


/*
 * Pack the data portion.
 */
bool E133PDU::PackData(uint8_t *data, unsigned int &length) const {
  if (m_pdu)
    return m_pdu->Pack(data, length);
  length = 0;
  return true;
}


/*
 * Pack the header into a buffer.
 */
void E133PDU::PackHeader(OutputStream *stream) const {
  E133Header::e133_pdu_header header;
  strncpy(header.source, m_header.Source().data(),
          E133Header::SOURCE_NAME_LEN);
  header.sequence = HostToNetwork(m_header.Sequence());
  header.endpoint = HostToNetwork(m_header.Endpoint());
  header.options = static_cast<uint8_t>(
      m_header.RxAcknowledge() ? E133Header::E133_RX_ACK_MASK : 0);
  stream->Write(reinterpret_cast<uint8_t*>(&header),
                sizeof(E133Header::e133_pdu_header));
}


/*
 * Pack the data into a buffer
 */
void E133PDU::PackData(OutputStream *stream) const {
  if (m_pdu)
    m_pdu->Write(stream);
}
}  // ola
}  // e131
}  // plugin
