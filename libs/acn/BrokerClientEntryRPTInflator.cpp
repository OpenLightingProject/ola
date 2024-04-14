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
 * BrokerClientEntryRPTInflator.cpp
 * The Inflator for BrokerClientEntryRPT PDU
 * Copyright (C) 2023 Peter Newman
 */

#include <iostream>
#include <memory>

#include "ola/Logging.h"
#include "ola/acn/CID.h"
#include "ola/e133/E133Helper.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/UID.h"
#include "include/ola/strings/Format.h"
#include "libs/acn/BrokerClientEntryRPTInflator.h"

namespace ola {
namespace acn {

using ola::acn::CID;
using ola::network::NetworkToHost;
using ola::rdm::UID;
using ola::strings::IntToString;

/**
 * Set a BrokerClientEntryRPTHandler to run when receiving a Broker Client
 * Entry RPT message.
 * @param handler the callback to invoke when there is a Broker Client Entry
 * RPT.
 */
void BrokerClientEntryRPTInflator::SetBrokerClientEntryRPTHandler(
    BrokerClientEntryRPTHandler *handler) {
  m_broker_client_entry_rpt_handler.reset(handler);
}


unsigned int BrokerClientEntryRPTInflator::InflatePDUBlock(
    OLA_UNUSED HeaderSet *headers,
    const uint8_t *data,
    unsigned int len) {
  broker_client_entry_rpt_pdu_data pdu_data;
  if (len > sizeof(pdu_data)) {
    OLA_WARN << "Got too much data, received " << len << " only expecting "
             << sizeof(pdu_data);
    return 0;
  }

  memcpy(reinterpret_cast<uint8_t*>(&pdu_data), data, sizeof(pdu_data));

  OLA_DEBUG << "Client Entry RPT from " << CID::FromData(pdu_data.client_cid)
            << " (" << UID(pdu_data.client_uid) << ") of RPT Client Type "
            << IntToString(pdu_data.rpt_client_type);

  ola::e133::E133RPTClientTypeCode client_type;
  if (!ola::e133::IntToRPTClientType(pdu_data.rpt_client_type, &client_type)) {
    OLA_WARN << "Unknown E1.33 RPT Client Type code "
             << IntToString(pdu_data.rpt_client_type);
  }

  BrokerClientEntryRPT client_entry(CID::FromData(pdu_data.client_cid),
                                    UID(pdu_data.client_uid),
                                    client_type,
                                    CID::FromData(pdu_data.binding_cid));

  if (m_broker_client_entry_rpt_handler.get()) {
    m_broker_client_entry_rpt_handler->Run(headers,
                                           client_entry);
  } else {
    OLA_WARN << "No Broker Client Entry RPT handler defined!";
  }
  return sizeof(pdu_data);
}
}  // namespace acn
}  // namespace ola
