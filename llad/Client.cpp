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
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <google/protobuf/stubs/common.h>
#include <lla/Logging.h>
#include "common/protocol/Lla.pb.h"
#include "Client.h"

namespace lla {

using google::protobuf::NewCallback;
using lla::rpc::SimpleRpcController;

bool Client::SendDMX(unsigned int universe_id, const DmxBuffer &buffer) {
  if (!m_client_stub) {
    LLA_FATAL << "client_stub is null";
    return false;
  }

  SimpleRpcController *controller = new SimpleRpcController();
  lla::proto::DmxData dmx_data;
  lla::proto::Ack *ack = new lla::proto::Ack();

  dmx_data.set_universe(universe_id);
  dmx_data.set_data(buffer.Get());

  m_client_stub->UpdateDmxData(
      controller,
      &dmx_data,
      ack,
      NewCallback(this, &lla::Client::SendDMXCallback, controller, ack));
  return true;
}


void Client::SendDMXCallback(SimpleRpcController *controller,
                             lla::proto::Ack *reply) {
  delete controller;
  delete reply;
}

void Client::SetDMX(const DmxBuffer &buffer) {
  m_buffer = buffer;
}

} //lla
