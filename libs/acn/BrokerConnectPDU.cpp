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
 * BrokerConnectPDU.cpp
 * The BrokerConnectPDU
 * Copyright (C) 2023 Peter Newman
 */

#include <algorithm>

#include "libs/acn/BrokerConnectPDU.h"

#include <ola/network/NetworkUtils.h>
#include <ola/acn/ACNVectors.h>

namespace ola {
namespace acn {

using ola::io::OutputStream;
using ola::network::HostToNetwork;
using std::min;
using std::string;

unsigned int BrokerConnectPDU::DataSize() const {
  //broker_connect_pdu_data data;
  return static_cast<unsigned int>(sizeof(broker_connect_pdu_data));
}

bool BrokerConnectPDU::PackData(uint8_t *data, unsigned int *length) const {
  broker_connect_pdu_data pdu_data;

  size_t client_scope_str_len = min(m_client_scope.size(), sizeof(pdu_data.client_scope));
  strncpy(pdu_data.client_scope, m_client_scope.c_str(), client_scope_str_len);
  memset(pdu_data.client_scope + client_scope_str_len, 0,
         (sizeof(pdu_data.client_scope) - client_scope_str_len));

  pdu_data.e133_version = HostToNetwork(m_e133_version);

  size_t search_domain_str_len = min(m_search_domain.size(), sizeof(pdu_data.search_domain));
  strncpy(pdu_data.search_domain, m_search_domain.c_str(), search_domain_str_len);
  memset(pdu_data.search_domain + search_domain_str_len, 0,
         (sizeof(pdu_data.search_domain) - search_domain_str_len));

  uint8_t connection = 0;
  if (m_incremental_updates) {
    connection |= CONNECTION_INCREMENTAL_UPDATES;
  }
  pdu_data.connection = HostToNetwork(connection);
  *length = static_cast<unsigned int>(sizeof(broker_connect_pdu_data));

  memcpy(data, &pdu_data, *length);
  return true;
}

void BrokerConnectPDU::PackData(ola::io::OutputStream *stream) const {
  broker_connect_pdu_data pdu_data;

  size_t client_scope_str_len = min(m_client_scope.size(), sizeof(pdu_data.client_scope));
  strncpy(pdu_data.client_scope, m_client_scope.c_str(), client_scope_str_len);
  memset(pdu_data.client_scope + client_scope_str_len, 0,
         (sizeof(pdu_data.client_scope) - client_scope_str_len));

  pdu_data.e133_version = HostToNetwork(m_e133_version);

  size_t search_domain_str_len = min(m_search_domain.size(), sizeof(pdu_data.search_domain));
  strncpy(pdu_data.search_domain, m_search_domain.c_str(), search_domain_str_len);
  memset(pdu_data.search_domain + search_domain_str_len, 0,
         (sizeof(pdu_data.search_domain) - search_domain_str_len));

  uint8_t connection = 0;
  if (m_incremental_updates) {
    connection |= CONNECTION_INCREMENTAL_UPDATES;
  }
  pdu_data.connection = HostToNetwork(connection);

  stream->Write(reinterpret_cast<uint8_t*>(&pdu_data),
                static_cast<unsigned int>(sizeof(broker_connect_pdu_data)));
}

void BrokerConnectPDU::PrependPDU(ola::io::IOStack *stack,
                                  const string &client_scope,
                                  uint16_t e133_version,
                                  const string &search_domain,
                                  bool incremental_updates) {
  broker_connect_pdu_data pdu_data;

  size_t client_scope_str_len = min(client_scope.size(), sizeof(pdu_data.client_scope));
  strncpy(pdu_data.client_scope, client_scope.c_str(), client_scope_str_len);
  memset(pdu_data.client_scope + client_scope_str_len, 0,
         (sizeof(pdu_data.client_scope) - client_scope_str_len));

  pdu_data.e133_version = HostToNetwork(e133_version);

  size_t search_domain_str_len = min(search_domain.size(), sizeof(pdu_data.search_domain));
  strncpy(pdu_data.search_domain, search_domain.c_str(), search_domain_str_len);
  memset(pdu_data.search_domain + search_domain_str_len, 0,
         (sizeof(pdu_data.search_domain) - search_domain_str_len));

  uint8_t connection = 0;
  if (incremental_updates) {
    connection |= CONNECTION_INCREMENTAL_UPDATES;
  }
  pdu_data.connection = HostToNetwork(connection);
  stack->Write(reinterpret_cast<uint8_t*>(&pdu_data),
               static_cast<unsigned int>(sizeof(broker_connect_pdu_data)));
  uint16_t vector = HostToNetwork(static_cast<uint16_t>(VECTOR_BROKER_CONNECT));
  stack->Write(reinterpret_cast<uint8_t*>(&vector), sizeof(vector));
  PrependFlagsAndLength(stack, VFLAG_MASK | HFLAG_MASK | DFLAG_MASK, true);
}
}  // namespace acn
}  // namespace ola
