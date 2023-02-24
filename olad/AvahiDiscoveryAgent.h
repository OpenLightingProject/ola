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
 * AvahiDiscoveryAgent.h
 * The Avahi implementation of DiscoveryAgentInterface.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef OLAD_AVAHIDISCOVERYAGENT_H_
#define OLAD_AVAHIDISCOVERYAGENT_H_

#include <avahi-client/client.h>
#include <avahi-common/thread-watch.h>
#include <avahi-client/publish.h>

#include <ola/base/Macro.h>
#include <ola/util/Backoff.h>
#include <map>
#include <string>
#include <vector>

#include "olad/DiscoveryAgent.h"

namespace ola {

/**
 * @brief An implementation of DiscoveryAgentInterface that uses the Avahi
 * client library.
 */
class AvahiDiscoveryAgent : public DiscoveryAgentInterface {
 public:
  AvahiDiscoveryAgent();
  ~AvahiDiscoveryAgent();

  bool Init();

  void RegisterService(const std::string &service_name,
                       const std::string &type,
                       uint16_t port,
                       const RegisterOptions &options);

  /**
   * @brief Called when the Avahi client state changes
   */
  void ClientStateChanged(AvahiClientState state, AvahiClient *client);

  /**
   * @brief Called when an entry group state changes.
   */
  void GroupStateChanged(const std::string &service_key,
                         AvahiEntryGroup *group,
                         AvahiEntryGroupState state);

  /**
   * @brief Called when the reconnect timeout expires.
   */
  void ReconnectTimeout();

 private:
  // The structure used to track services.
  struct ServiceEntry : public RegisterOptions {
   public:
    const std::string service_name;
    // This may differ from the service name if there was a collision.
    std::string actual_service_name;
    const uint16_t port;
    AvahiEntryGroup *group;
    AvahiEntryGroupState state;
    struct EntryGroupParams *params;

    ServiceEntry(const std::string &service_name,
                 const std::string &type_spec,
                 uint16_t port,
                 const RegisterOptions &options);

    std::string key() const;
    const std::string &type() const { return m_type; }
    const std::vector<std::string> sub_types() const { return m_sub_types; }

   private:
    std::string m_type_spec;   // type[,subtype]
    std::string m_type;
    std::vector<std::string> m_sub_types;
  };

  typedef std::map<std::string, ServiceEntry*> Services;

  AvahiThreadedPoll *m_threaded_poll;
  AvahiClient *m_client;
  AvahiTimeout *m_reconnect_timeout;
  Services m_services;
  BackoffGenerator m_backoff;

  bool InternalRegisterService(ServiceEntry *service);
  void CreateNewClient();
  void UpdateServices();
  void DeregisterAllServices();
  void SetUpReconnectTimeout();
  bool RenameAndRegister(ServiceEntry *service);

  static std::string ClientStateToString(AvahiClientState state);
  static std::string GroupStateToString(AvahiEntryGroupState state);

  DISALLOW_COPY_AND_ASSIGN(AvahiDiscoveryAgent);
};
}  // namespace ola
#endif  // OLAD_AVAHIDISCOVERYAGENT_H_
