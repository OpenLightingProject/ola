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
#include "common/rpc/SimpleRpcController.h"
#include "common/protocol/Lla.pb.h"
#include "Client.h"

namespace lla {

using google::protobuf::NewCallback;
using lla::rpc::SimpleRpcController;

int Client::SendDMX(unsigned int universe_id,
                    uint8_t *data,
                    unsigned int length) {

  SimpleRpcController *controller = new SimpleRpcController();
  lla::proto::DmxData *dmx_data = new lla::proto::DmxData();
  lla::proto::Ack *ack = new lla::proto::Ack();
  string dmx_string;
  dmx_string.append((char*) data, length);

  dmx_data->set_universe(universe_id);
  dmx_data->set_data(dmx_string);

  m_client_stub->UpdateDmxData(controller,
                               dmx_data,
                               ack,
                               NewCallback(this, &lla::Client::SendDMXCallback));

  return 0;
}


void Client::SendDMXCallback() {

}

} //lla
