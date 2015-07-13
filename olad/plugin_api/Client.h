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
 * Client.h
 * Header file for the olad Client class.
 * Copyright (C) 2005 Simon Newton
 */

#ifndef OLAD_PLUGIN_API_CLIENT_H_
#define OLAD_PLUGIN_API_CLIENT_H_

#include <map>
#include <memory>
#include "common/rpc/RpcController.h"
#include "ola/base/Macro.h"
#include "ola/rdm/UID.h"
#include "olad/DmxSource.h"

namespace ola {
namespace proto {
class OlaClientService_Stub;
class Ack;
}
}

namespace ola {

/**
 * @brief Represents a connected OLA client on the OLA server side.
 *
 * This stores the state of the client (i.e. DMX data) and allows us to push
 * DMX updates to the client via the OlaClientService_Stub.
 */
class Client {
 public :
  /**
   * @brief Create a new client.
   * @param client_stub The OlaClientService_Stub to use to communicate with
   *   the client. Ownership is transferred to the client.
   * @param uid The default UID to use for this client. The client may set its
   *   own UID later.
   */
  Client(ola::proto::OlaClientService_Stub *client_stub,
         const ola::rdm::UID &uid);

  virtual ~Client();

  /**
   * @brief Push a DMX update to this client.
   * @param universe_id the universe the DMX data belongs to
   * @param priority the priority of the DMX data
   * @param buffer the DMX data.
   * @return true if the update was sent, false otherwise
   */
  virtual bool SendDMX(unsigned int universe_id, uint8_t priority,
                       const DmxBuffer &buffer);

  /**
   * @brief Called when this client sends us new data
   * @param universe the id of the universe for the new data
   * @param source the new DMX data.
   */
  void DMXReceived(unsigned int universe, const DmxSource &source);

  /**
   * @brief Get the most recent DMX data received from this client.
   * @param universe the id of the universe we're interested in
   */
  const DmxSource SourceData(unsigned int universe) const;

  /**
   * @brief Return the UID associated with this client.
   * @returns The client's UID.
   *
   * This is normally the UID passed in the constructor, unless the client
   * itself overrides the UID.
   */
  ola::rdm::UID GetUID() const;

  /**
   * @brief Set the UID for the client.
   * @param uid the new UID to use for this client.
   */
  void SetUID(const ola::rdm::UID &uid);

 private:
  void SendDMXCallback(ola::rpc::RpcController *controller,
                       ola::proto::Ack *ack);

  std::auto_ptr<class ola::proto::OlaClientService_Stub> m_client_stub;
  std::map<unsigned int, DmxSource> m_data_map;
  ola::rdm::UID m_uid;

  DISALLOW_COPY_AND_ASSIGN(Client);
};
}  // namespace ola
#endif  // OLAD_PLUGIN_API_CLIENT_H_
