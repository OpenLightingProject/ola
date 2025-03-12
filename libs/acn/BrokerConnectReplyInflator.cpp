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
 * BrokerConnectReplyInflator.cpp
 * The Inflator for BrokerConnectReply PDU
 * Copyright (C) 2023 Peter Newman
 */

#include <iostream>
#include <memory>

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/UID.h"
#include "libs/acn/BrokerConnectReplyInflator.h"

namespace ola {
namespace acn {

using ola::network::NetworkToHost;
using ola::rdm::UID;

/**
 * Set a BrokerConnectReplyHandler to run when receiving a Broker Connect Reply
 * message.
 * @param handler the callback to invoke when there is a Broker Connect Reply.
 */
void BrokerConnectReplyInflator::SetBrokerConnectReplyHandler(
    BrokerConnectReplyHandler *handler) {
  m_broker_connect_reply_handler.reset(handler);
}

unsigned int BrokerConnectReplyInflator::InflatePDUBlock(HeaderSet *headers,
                                                         const uint8_t *data,
                                                         unsigned int len) {
  broker_connect_reply_pdu_data pdu_data;
  if (len > sizeof(pdu_data)) {
    OLA_WARN << "Got too much data, received " << len << " only expecting "
             << sizeof(pdu_data);
    return 0;
  }

  memcpy(reinterpret_cast<uint8_t*>(&pdu_data), data, sizeof(pdu_data));

  OLA_DEBUG << "Connect reply from " << UID(pdu_data.broker_uid) << " for "
            << UID(pdu_data.client_uid) << " with connection code "
            << pdu_data.connection_code << " using E1.33 version "
            << NetworkToHost(pdu_data.e133_version);

  BrokerConnectReply reply(pdu_data.connection_code,
                           NetworkToHost(pdu_data.e133_version),
                           UID(pdu_data.broker_uid),
                           UID(pdu_data.client_uid));

  if (m_broker_connect_reply_handler.get()) {
    m_broker_connect_reply_handler->Run(headers, reply);
  } else {
    OLA_WARN << "No Broker Connect Reply handler defined!";
  }
  return sizeof(pdu_data);
}
}  // namespace acn
}  // namespace ola
