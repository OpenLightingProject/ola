/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Client.cpp
 * Represents a connected client.
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <google/protobuf/stubs/common.h>
#include <ola/Logging.h>
#include "common/protocol/Ola.pb.h"
#include "Client.h"

namespace ola {

using google::protobuf::NewCallback;
using ola::rpc::SimpleRpcController;

Client::~Client() {
  m_data_map.clear();
}


/*
 * Send a DMX Update to this client
 * @param universe the universe_id for this data
 * @param buffer the DmxBuffer with the data
 * @return true if the update was sent, false otherwise
 */
bool Client::SendDMX(unsigned int universe, const DmxBuffer &buffer) {
  if (!m_client_stub) {
    OLA_FATAL << "client_stub is null";
    return false;
  }

  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::DmxData dmx_data;
  ola::proto::Ack *ack = new ola::proto::Ack();

  dmx_data.set_universe(universe);
  dmx_data.set_data(buffer.Get());

  m_client_stub->UpdateDmxData(
      controller,
      &dmx_data,
      ack,
      NewCallback(this, &ola::Client::SendDMXCallback, controller, ack));
  return true;
}


/*
 * Called when UpdateDmxData completes
 */
void Client::SendDMXCallback(SimpleRpcController *controller,
                             ola::proto::Ack *reply) {
  delete controller;
  delete reply;
}


/*
 * Called when this client sends us new data
 * @param universe the id of the universe for the new data
 * @param buffer the new data
 */
void Client::DMXRecieved(unsigned int universe, const DmxBuffer &buffer) {
  map<unsigned int, DmxBuffer>::iterator iter = m_data_map.find(universe);

  if (iter != m_data_map.end()) {
    iter->second = buffer;
  } else {
    std::pair<unsigned int, DmxBuffer> pair(universe, buffer);
    m_data_map.insert(pair);
  }
}


/*
 * Return the last dmx data sent by this client
 * @param universe the id of the universe we're interested in
 */
const DmxBuffer Client::GetDMX(unsigned int universe) const {
  map<unsigned int, DmxBuffer>::const_iterator iter =
    m_data_map.find(universe);

  if (iter != m_data_map.end()) {
    return iter->second;
  } else {
    DmxBuffer empty_buffer;
    return empty_buffer;
  }
}

} //ola
