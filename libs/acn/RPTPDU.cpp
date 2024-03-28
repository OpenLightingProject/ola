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
 * RPTPDU.cpp
 * The E1.33 RPT PDU
 * Copyright (C) 2024 Peter Newman
 */


#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/network/NetworkUtils.h"
#include "libs/acn/RPTPDU.h"

namespace ola {
namespace acn {

using ola::io::OutputStream;
using ola::network::HostToNetwork;

/*
 * Size of the header portion.
 */
unsigned int RPTPDU::HeaderSize() const {
  return sizeof(RPTHeader::rpt_pdu_header);
}


/*
 * Size of the data portion
 */
unsigned int RPTPDU::DataSize() const {
  return m_pdu ? m_pdu->Size() : 0;
}


/*
 * Pack the header portion.
 */
bool RPTPDU::PackHeader(uint8_t *data, unsigned int *length) const {
  unsigned int header_size = HeaderSize();

  if (*length < header_size) {
    OLA_WARN << "RPTPDU::PackHeader: buffer too small, got " << *length
             << " required " << header_size;
    *length = 0;
    return false;
  }

  RPTHeader::rpt_pdu_header header;
  m_header.SourceUID().Pack(header.source_uid, sizeof(header.source_uid));
  header.source_endpoint = HostToNetwork(m_header.SourceEndpoint());
  m_header.DestinationUID().Pack(header.destination_uid,
                                 sizeof(header.destination_uid));
  header.destination_endpoint = HostToNetwork(m_header.DestinationEndpoint());
  header.sequence = HostToNetwork(m_header.Sequence());
  *length = sizeof(RPTHeader::rpt_pdu_header);
  memcpy(data, &header, *length);
  return true;
}


/*
 * Pack the data portion.
 */
bool RPTPDU::PackData(uint8_t *data, unsigned int *length) const {
  if (m_pdu)
    return m_pdu->Pack(data, length);
  *length = 0;
  return true;
}


/*
 * Pack the header into a buffer.
 */
void RPTPDU::PackHeader(OutputStream *stream) const {
  RPTHeader::rpt_pdu_header header;
  m_header.SourceUID().Pack(header.source_uid, sizeof(header.source_uid));
  header.source_endpoint = HostToNetwork(m_header.SourceEndpoint());
  m_header.DestinationUID().Pack(header.destination_uid,
                                 sizeof(header.destination_uid));
  header.destination_endpoint = HostToNetwork(m_header.DestinationEndpoint());
  header.sequence = HostToNetwork(m_header.Sequence());
  stream->Write(reinterpret_cast<uint8_t*>(&header),
                sizeof(RPTHeader::rpt_pdu_header));
}


/*
 * Pack the data into a buffer
 */
void RPTPDU::PackData(OutputStream *stream) const {
  if (m_pdu)
    m_pdu->Write(stream);
}


void RPTPDU::PrependPDU(ola::io::IOStack *stack,
                        uint32_t vector,
                        const ola::rdm::UID &source_uid,
                        uint16_t source_endpoint,
                        const ola::rdm::UID &destination_uid,
                        uint16_t destination_endpoint,
                        uint32_t sequence_number) {
  RPTHeader::rpt_pdu_header header;
  source_uid.Pack(header.source_uid, sizeof(header.source_uid));
  header.source_endpoint = HostToNetwork(source_endpoint);
  destination_uid.Pack(header.destination_uid, sizeof(header.destination_uid));
  header.destination_endpoint = HostToNetwork(destination_endpoint);
  header.sequence = HostToNetwork(sequence_number);
  stack->Write(reinterpret_cast<uint8_t*>(&header),
               sizeof(RPTHeader::rpt_pdu_header));

  vector = HostToNetwork(vector);
  stack->Write(reinterpret_cast<uint8_t*>(&vector), sizeof(vector));
  PrependFlagsAndLength(stack, VFLAG_MASK | HFLAG_MASK | DFLAG_MASK, true);
}
}  // namespace acn
}  // namespace ola
