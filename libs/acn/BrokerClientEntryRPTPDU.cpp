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
 * BrokerClientEntryRPTPDU.cpp
 * The BrokerClientEntryRPTPDU
 * Copyright (C) 2023 Peter Newman
 */


#include "ola/Logging.h"
#include "ola/acn/CID.h"
#include "ola/base/Array.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/UID.h"
#include "libs/acn/BrokerClientEntryRPTPDU.h"
#include "libs/acn/BrokerClientEntryPDU.h"

namespace ola {
namespace acn {

using ola::acn::CID;
using ola::io::OutputStream;
using ola::network::HostToNetwork;
using ola::rdm::UID;

/*
 * Size of the header portion.
 */
unsigned int BrokerClientEntryRPTPDU::HeaderSize() const {
  return sizeof(BrokerClientEntryHeader::broker_client_entry_pdu_header);
}


/*
 * Size of the data portion
 */
unsigned int BrokerClientEntryRPTPDU::DataSize() const {
  return sizeof(broker_client_entry_rpt_pdu_data);
}


/*
 * Pack the header portion.
 */
bool BrokerClientEntryRPTPDU::PackHeader(uint8_t *data,
                                         unsigned int *length) const {
  unsigned int header_size = HeaderSize();

  if (*length < header_size) {
    OLA_WARN << "BrokerClientEntryRPTPDU::PackHeader: buffer too small, got "
             << *length << " required " << header_size;
    *length = 0;
    return false;
  }

  BrokerClientEntryHeader::broker_client_entry_pdu_header header;
  m_header.ClientCid().Pack(header.client_cid);
  *length = sizeof(BrokerClientEntryHeader::broker_client_entry_pdu_header);
  memcpy(data, &header, *length);
  return true;
}


/*
 * Pack the data portion.
 */
bool BrokerClientEntryRPTPDU::PackData(uint8_t *data,
                                       unsigned int *length) const {
  unsigned int data_size = DataSize();

  if (*length < data_size) {
    OLA_WARN << "BrokerClientEntryRPTPDU::PackData: buffer too small, got "
             << *length << " required " << data_size;
    *length = 0;
    return false;
  }

  broker_client_entry_rpt_pdu_data pdu_data;

  m_client_uid.Pack(pdu_data.client_uid, sizeof(pdu_data.client_uid));
  pdu_data.rpt_client_type = m_rpt_client_type;
  m_binding_cid.Pack(pdu_data.binding_cid);

  *length = sizeof(broker_client_entry_rpt_pdu_data);
  memcpy(data, &pdu_data, *length);
  return true;
}


/*
 * Pack the header into a buffer.
 */
void BrokerClientEntryRPTPDU::PackHeader(OutputStream *stream) const {
  BrokerClientEntryHeader::broker_client_entry_pdu_header header;
  m_header.ClientCid().Pack(header.client_cid);
  stream->Write(
      reinterpret_cast<uint8_t*>(&header),
      sizeof(BrokerClientEntryHeader::broker_client_entry_pdu_header));
}


/*
 * Pack the data into a buffer
 */
void BrokerClientEntryRPTPDU::PackData(OutputStream *stream) const {
  broker_client_entry_rpt_pdu_data pdu_data;

  m_client_uid.Pack(pdu_data.client_uid, sizeof(pdu_data.client_uid));
  pdu_data.rpt_client_type = m_rpt_client_type;
  m_binding_cid.Pack(pdu_data.binding_cid);

  stream->Write(
      reinterpret_cast<uint8_t*>(&pdu_data),
      static_cast<unsigned int>(sizeof(broker_client_entry_rpt_pdu_data)));
}


void BrokerClientEntryRPTPDU::PrependPDU(ola::io::IOStack *stack,
                                         uint32_t vector,
                                         const ola::acn::CID &client_cid,
                                         const ola::rdm::UID &client_uid,
                                         uint8_t rpt_client_type,
                                         const ola::acn::CID &binding_cid) {
  broker_client_entry_rpt_pdu_data pdu_data;

  client_uid.Pack(pdu_data.client_uid, sizeof(pdu_data.client_uid));
  pdu_data.rpt_client_type = rpt_client_type;
  binding_cid.Pack(pdu_data.binding_cid);

  stack->Write(
      reinterpret_cast<uint8_t*>(&pdu_data),
      static_cast<unsigned int>(sizeof(broker_client_entry_rpt_pdu_data)));

  BrokerClientEntryPDU::PrependPDU(stack, vector, client_cid);
}
}  // namespace acn
}  // namespace ola
