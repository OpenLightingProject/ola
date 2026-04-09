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
 * BrokerConnectReplyInflator.h
 * Interface for the BrokerConnectReplyInflator class.
 * Copyright (C) 2023 Peter Newman
 */

#ifndef LIBS_ACN_BROKERCONNECTREPLYINFLATOR_H_
#define LIBS_ACN_BROKERCONNECTREPLYINFLATOR_H_

#include "ola/Callback.h"
#include "ola/acn/ACNVectors.h"
#include "ola/rdm/UID.h"
#include "libs/acn/BaseInflator.h"

namespace ola {
namespace acn {

class BrokerConnectReplyInflator: public BaseInflator {
  friend class BrokerConnectReplyInflatorTest;

 public:
  struct BrokerConnectReply {
    BrokerConnectReply(uint16_t _connection_code,
                       uint16_t _e133_version,
                       const ola::rdm::UID &_broker_uid,
                       const ola::rdm::UID &_client_uid)
       : connection_code(_connection_code),
         e133_version(_e133_version),
         broker_uid(_broker_uid),
         client_uid(_client_uid) {
    }
    uint16_t connection_code;
    uint16_t e133_version;
    ola::rdm::UID broker_uid;
    ola::rdm::UID client_uid;
  };


  // These are pointers so the callers don't have to pull in all the headers.
  typedef ola::Callback2<void,
                         const HeaderSet*,  // the HeaderSet
                         const BrokerConnectReply&  // Broker Connect Reply Data
                        > BrokerConnectReplyHandler;

  BrokerConnectReplyInflator()
    : BaseInflator() {
  }
  ~BrokerConnectReplyInflator() {}

  uint32_t Id() const { return ola::acn::VECTOR_BROKER_CONNECT_REPLY; }

  PACK(
  struct broker_connect_reply_pdu_data_s {
    uint16_t connection_code;
    uint16_t e133_version;
    uint8_t broker_uid[ola::rdm::UID::LENGTH];
    uint8_t client_uid[ola::rdm::UID::LENGTH];
  });
  typedef struct broker_connect_reply_pdu_data_s broker_connect_reply_pdu_data;

  void SetBrokerConnectReplyHandler(BrokerConnectReplyHandler *handler);

 protected:
  // The 'header' is 0 bytes in length.
  bool DecodeHeader(HeaderSet*,
                    const uint8_t*,
                    unsigned int,
                    unsigned int *bytes_used) {
    *bytes_used = 0;
    return true;
  }

  void ResetHeaderField() {}  // namespace noop

  unsigned int InflatePDUBlock(HeaderSet *headers,
                               const uint8_t *data,
                               unsigned int len);

 private:
  std::auto_ptr<BrokerConnectReplyHandler> m_broker_connect_reply_handler;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_BROKERCONNECTREPLYINFLATOR_H_
