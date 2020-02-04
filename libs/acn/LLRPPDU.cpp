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
 * LLRPPDU.cpp
 * The LLRPPDU
 * Copyright (C) 2020 Peter Newman
 */


#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/network/NetworkUtils.h"
#include "libs/acn/LLRPPDU.h"

namespace ola {
namespace acn {

using ola::io::OutputStream;
using ola::network::HostToNetwork;

/*
 * Size of the header portion.
 */
unsigned int LLRPPDU::HeaderSize() const {
  return sizeof(LLRPHeader::llrp_pdu_header);
}


/*
 * Size of the data portion
 */
unsigned int LLRPPDU::DataSize() const {
  return m_pdu ? m_pdu->Size() : 0;
}


/*
 * Pack the header portion.
 */
bool LLRPPDU::PackHeader(uint8_t *data, unsigned int *length) const {
  unsigned int header_size = HeaderSize();

  if (*length < header_size) {
    OLA_WARN << "LLRPPDU::PackHeader: buffer too small, got " << *length
             << " required " << header_size;
    *length = 0;
    return false;
  }

  LLRPHeader::llrp_pdu_header header;
  m_header.DestinationCid().Pack(header.destination_cid);
  header.transaction_number = HostToNetwork(m_header.TransactionNumber());
  *length = sizeof(LLRPHeader::llrp_pdu_header);
  memcpy(data, &header, *length);
  return true;
}


/*
 * Pack the data portion.
 */
bool LLRPPDU::PackData(uint8_t *data, unsigned int *length) const {
  if (m_pdu)
    return m_pdu->Pack(data, length);
  *length = 0;
  return true;
}


/*
 * Pack the header into a buffer.
 */
void LLRPPDU::PackHeader(OutputStream *stream) const {
  LLRPHeader::llrp_pdu_header header;
  m_header.DestinationCid().Pack(header.destination_cid);
  header.transaction_number = HostToNetwork(m_header.TransactionNumber());
  stream->Write(reinterpret_cast<uint8_t*>(&header),
                sizeof(LLRPHeader::llrp_pdu_header));
}


/*
 * Pack the data into a buffer
 */
void LLRPPDU::PackData(OutputStream *stream) const {
  if (m_pdu)
    m_pdu->Write(stream);
}


void LLRPPDU::PrependPDU(ola::io::IOStack *stack,
                         uint32_t vector,
                         const ola::acn::CID &destination_cid,
                         uint32_t transaction_number) {
  LLRPHeader::llrp_pdu_header header;
  destination_cid.Pack(header.destination_cid);
  header.transaction_number = HostToNetwork(transaction_number);
  stack->Write(reinterpret_cast<uint8_t*>(&header),
               sizeof(LLRPHeader::llrp_pdu_header));

  vector = HostToNetwork(vector);
  stack->Write(reinterpret_cast<uint8_t*>(&vector), sizeof(vector));
  PrependFlagsAndLength(stack, VFLAG_MASK | HFLAG_MASK | DFLAG_MASK, true);
}
}  // namespace acn
}  // namespace ola
