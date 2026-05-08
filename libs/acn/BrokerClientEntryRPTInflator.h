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
 * BrokerClientEntryRPTInflator.h
 * Interface for the BrokerClientEntryRPTInflator class.
 * Copyright (C) 2023 Peter Newman
 */

#ifndef LIBS_ACN_BROKERCLIENTENTRYRPTINFLATOR_H_
#define LIBS_ACN_BROKERCLIENTENTRYRPTINFLATOR_H_

#include "ola/Callback.h"
#include "ola/acn/ACNVectors.h"
#include "ola/e133/E133Enums.h"
#include "ola/rdm/UID.h"
#include "libs/acn/BaseInflator.h"

namespace ola {
namespace acn {

class BrokerClientEntryRPTInflator: public BaseInflator {
  friend class BrokerClientEntryRPTInflatorTest;

 public:
  struct BrokerClientEntryRPT {
    BrokerClientEntryRPT(const ola::acn::CID &_client_cid,
                         const ola::rdm::UID &_client_uid,
                         ola::e133::E133RPTClientTypeCode _client_type_code,
                         const ola::acn::CID &_binding_cid)
       : client_cid(_client_cid),
         client_uid(_client_uid),
         client_type_code(_client_type_code),
         binding_cid(_binding_cid) {
    }
    ola::acn::CID client_cid;
    ola::rdm::UID client_uid;
    ola::e133::E133RPTClientTypeCode client_type_code;
    ola::acn::CID binding_cid;
  };


  // These are pointers so the callers don't have to pull in all the headers.
  typedef ola::Callback2<void,
                         const HeaderSet*,  // the HeaderSet
                         const BrokerClientEntryRPT&  // RPT Client Entry Data
                        > BrokerClientEntryRPTHandler;

  BrokerClientEntryRPTInflator()
      : BaseInflator() {
  }
  ~BrokerClientEntryRPTInflator() {}

  uint32_t Id() const { return ola::acn::CLIENT_PROTOCOL_RPT; }

  PACK(
  struct broker_client_entry_rpt_pdu_data_s {
    uint8_t client_cid[ola::acn::CID::CID_LENGTH];
    uint8_t client_uid[ola::rdm::UID::LENGTH];
    uint8_t rpt_client_type;
    uint8_t binding_cid[ola::acn::CID::CID_LENGTH];
  });
  typedef struct broker_client_entry_rpt_pdu_data_s
      broker_client_entry_rpt_pdu_data;

  void SetBrokerClientEntryRPTHandler(BrokerClientEntryRPTHandler *handler);

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
  std::unique_ptr<BrokerClientEntryRPTHandler> m_broker_client_entry_rpt_handler;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_BROKERCLIENTENTRYRPTINFLATOR_H_
