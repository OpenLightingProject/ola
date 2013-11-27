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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * client.h
 * Header file for the client class
 * Copyright (C) 2005 Simon Newton
 */

#ifndef OLAD_CLIENT_H_
#define OLAD_CLIENT_H_

#include <map>
#include "common/rpc/RpcController.h"
#include "ola/base/Macro.h"
#include "olad/DmxSource.h"

namespace ola {
namespace proto {
  class OlaClientService_Stub;
  class Ack;
}
}

namespace ola {

using std::map;
using ola::proto::OlaClientService_Stub;

class Client {
  public :
    explicit Client(OlaClientService_Stub *client_stub):
      m_client_stub(client_stub) {}
    virtual ~Client();
    virtual bool SendDMX(unsigned int universe_id, uint8_t priority,
                         const DmxBuffer &buffer);

    void SendDMXCallback(ola::rpc::RpcController *controller,
                         ola::proto::Ack *ack);
    void DMXReceived(unsigned int universe, const DmxSource &source);
    const DmxSource SourceData(unsigned int universe) const;
    class OlaClientService_Stub *Stub() const { return m_client_stub; }

  private:
    class OlaClientService_Stub *m_client_stub;
    map<unsigned int, DmxSource> m_data_map;

    DISALLOW_COPY_AND_ASSIGN(Client);
};
}  // namespace ola
#endif  // OLAD_CLIENT_H_
