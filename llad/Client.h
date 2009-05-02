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
 * client.h
 * Header file for the client class
 * Copyright (C) 2005 Simon Newton
 */

#ifndef LLA_CLIENT_H
#define LLA_CLIENT_H

#include <stdint.h>
#include "common/rpc/SimpleRpcController.h"

namespace lla {
namespace proto {
  class LlaClientService_Stub;
  class Ack;
}
}

namespace lla {

using namespace std;
using lla::proto::LlaClientService_Stub;

class Client {
  public :
    Client(LlaClientService_Stub *client_stub):
      m_client_stub(client_stub) {}
    virtual ~Client() {};
    virtual bool SendDMX(unsigned int universe_id,
                         const uint8_t *data,
                         unsigned int length);

    void SendDMXCallback(lla::rpc::SimpleRpcController *controller,
                         lla::proto::Ack *ack);
    class LlaClientService_Stub *Stub() const { return m_client_stub; }

  private:
    Client(const Client&);
    Client& operator=(const Client&);
    class LlaClientService_Stub *m_client_stub;
};

} //lla
#endif
