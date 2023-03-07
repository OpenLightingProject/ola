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
 * BrokerConnectPDU.h
 * The BrokerConnectPDU class
 * Copyright (C) 2023 Peter Newman
 */

#ifndef LIBS_ACN_BROKERCONNECTPDU_H_
#define LIBS_ACN_BROKERCONNECTPDU_H_

#include <ola/io/IOStack.h>
#include <ola/rdm/RDMEnums.h>

#include "libs/acn/PDU.h"

namespace ola {
namespace acn {

class BrokerConnectPDU : public PDU {
 public:
  explicit BrokerConnectPDU(unsigned int vector,
                            const std::string &client_scope,
                            uint16_t e133_version,
                            const std::string &search_domain,
                            bool incremental_updates):
    PDU(vector, TWO_BYTES, true),
    m_client_scope(client_scope),
    m_e133_version(e133_version),
    m_search_domain(search_domain),
    m_incremental_updates(incremental_updates) {}

  unsigned int HeaderSize() const { return 0; }
  bool PackHeader(OLA_UNUSED uint8_t *data,
                  unsigned int *length) const {
    *length = 0;
    return true;
  }
  void PackHeader(OLA_UNUSED ola::io::OutputStream *stream) const {}

  unsigned int DataSize() const;
  bool PackData(uint8_t *data, unsigned int *length) const;
  void PackData(ola::io::OutputStream *stream) const;

  static void PrependPDU(ola::io::IOStack *stack,
                         const std::string &client_scope,
                         uint16_t e133_version,
                         const std::string &search_domain,
                         bool incremental_updates);

  // bit masks for connection
  static const uint8_t CONNECTION_INCREMENTAL_UPDATES = 0x01;

  PACK(
  struct broker_connect_pdu_data_s {
    // Plus one to allow for the mandatory null
    char client_scope[ola::rdm::MAX_RDM_SCOPE_STRING_LENGTH + 1];
    uint16_t e133_version;
    char search_domain[ola::rdm::MAX_RDM_DOMAIN_NAME_LENGTH];
    uint8_t connection;
  });
  typedef struct broker_connect_pdu_data_s broker_connect_pdu_data;

 private:
  const std::string m_client_scope;
  uint16_t m_e133_version;
  const std::string m_search_domain;
  uint8_t m_incremental_updates;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_BROKERCONNECTPDU_H_
