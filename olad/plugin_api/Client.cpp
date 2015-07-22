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
 * Client.cpp
 * Represents a connected client.
 * Copyright (C) 2005 Simon Newton
 */

#include <map>
#include <utility>
#include "common/protocol/Ola.pb.h"
#include "common/protocol/OlaService.pb.h"
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/rdm/UID.h"
#include "ola/stl/STLUtils.h"
#include "olad/plugin_api/Client.h"

namespace ola {

using ola::rdm::UID;
using ola::rpc::RpcController;
using std::map;

Client::Client(ola::proto::OlaClientService_Stub *client_stub,
               const ola::rdm::UID &uid)
    : m_client_stub(client_stub),
      m_uid(uid) {
}

Client::~Client() {
  m_data_map.clear();
}

bool Client::SendDMX(unsigned int universe, uint8_t priority,
                     const DmxBuffer &buffer) {
  if (!m_client_stub.get()) {
    OLA_FATAL << "client_stub is null";
    return false;
  }

  RpcController *controller = new RpcController();
  ola::proto::DmxData dmx_data;
  ola::proto::Ack *ack = new ola::proto::Ack();

  dmx_data.set_priority(priority);
  dmx_data.set_universe(universe);
  dmx_data.set_data(buffer.Get());

  m_client_stub->UpdateDmxData(
      controller,
      &dmx_data,
      ack,
      ola::NewSingleCallback(this, &ola::Client::SendDMXCallback,
                             controller, ack));
  return true;
}

void Client::DMXReceived(unsigned int universe, const DmxSource &source) {
  STLReplace(&m_data_map, universe, source);
}

const DmxSource Client::SourceData(unsigned int universe) const {
  map<unsigned int, DmxSource>::const_iterator iter =
    m_data_map.find(universe);

  if (iter != m_data_map.end()) {
    return iter->second;
  } else {
    DmxSource source;
    return source;
  }
}

ola::rdm::UID Client::GetUID() const {
  return m_uid;
}

void Client::SetUID(const ola::rdm::UID &uid) {
  m_uid = uid;
}

/*
 * Called when UpdateDmxData completes.
 */
void Client::SendDMXCallback(RpcController *controller,
                             ola::proto::Ack *reply) {
  delete controller;
  delete reply;
}


}  // namespace ola
