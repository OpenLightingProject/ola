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
 * AvahiDiscoveryAgent.cpp
 * The Avahi implementation of DiscoveryAgentInterface.
 * Copyright (C) 2013 Simon Newton
 */

#include "olad/AvahiDiscoveryAgent.h"

#include <avahi-common/alternative.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/strlst.h>
#include <avahi-common/timeval.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/stl/STLUtils.h"
#include "ola/StringUtils.h"

namespace ola {

using std::string;
using std::vector;

static std::string MakeServiceKey(const std::string &service_name,
                                  const std::string &type) {
  return service_name + "." + type;
}

/*
 * The userdata passed to the entry_callback function.
 */
struct EntryGroupParams {
  AvahiDiscoveryAgent *agent;
  string key;

  EntryGroupParams(AvahiDiscoveryAgent *agent, const string &key)
      : agent(agent), key(key) {
  }
};

namespace {

/*
 * Called when the client state changes. This is called once from
 * the thread that calls avahi_client_new, and then from the poll thread.
 */
static void client_callback(AvahiClient *client,
                            AvahiClientState state,
                            void *data) {
  AvahiDiscoveryAgent *agent = reinterpret_cast<AvahiDiscoveryAgent*>(data);
  agent->ClientStateChanged(state, client);
}

/*
 * Called when the group state changes.
 */
static void entry_callback(AvahiEntryGroup *group,
                           AvahiEntryGroupState state,
                           void *data)  {
  if (!group) {
    return;
  }

  const EntryGroupParams *params = reinterpret_cast<const EntryGroupParams*>(
      data);
  if (!params) {
    OLA_FATAL << "entry_callback passed null userdata!";
    return;
  }
  params->agent->GroupStateChanged(params->key, group, state);
}

static void reconnect_callback(AvahiTimeout*, void *data) {
  AvahiDiscoveryAgent *agent = reinterpret_cast<AvahiDiscoveryAgent*>(data);
  agent->ReconnectTimeout();
}
}  // namespace

AvahiDiscoveryAgent::ServiceEntry::ServiceEntry(
    const std::string &service_name,
    const std::string &type_spec,
    uint16_t port,
    const RegisterOptions &options)
    : RegisterOptions(options),
      service_name(service_name),
      actual_service_name(service_name),
      port(port),
      group(NULL),
      state(AVAHI_ENTRY_GROUP_UNCOMMITED),
      params(NULL),
      m_type_spec(type_spec) {
  vector<string> tokens;
  StringSplit(type_spec, &tokens, ",");
  m_type = tokens[0];
  if (tokens.size() > 1) {
    copy(tokens.begin() + 1, tokens.end(), std::back_inserter(m_sub_types));
  }
}

string AvahiDiscoveryAgent::ServiceEntry::key() const {
  return MakeServiceKey(actual_service_name, m_type_spec);
}

AvahiDiscoveryAgent::AvahiDiscoveryAgent()
    : m_threaded_poll(avahi_threaded_poll_new()),
      m_client(NULL),
      m_reconnect_timeout(NULL),
      m_backoff(new ExponentialBackoffPolicy(TimeInterval(1, 0),
                                             TimeInterval(60, 0))) {
}

AvahiDiscoveryAgent::~AvahiDiscoveryAgent() {
  avahi_threaded_poll_stop(m_threaded_poll);

  if (m_reconnect_timeout) {
    avahi_threaded_poll_get(m_threaded_poll)->timeout_free(
        m_reconnect_timeout);
  }

  DeregisterAllServices();
  STLDeleteValues(&m_services);

  if (m_client) {
    avahi_client_free(m_client);
  }
  avahi_threaded_poll_free(m_threaded_poll);
}

bool AvahiDiscoveryAgent::Init() {
  CreateNewClient();

  if (m_threaded_poll) {
    avahi_threaded_poll_start(m_threaded_poll);
    return true;
  } else {
    return false;
  }
}

void AvahiDiscoveryAgent::RegisterService(const string &service_name,
                                          const string &type,
                                          uint16_t port,
                                          const RegisterOptions &options) {
  if (!(m_threaded_poll && m_client)) {
    return;
  }
  const string key = MakeServiceKey(service_name, type);
  avahi_threaded_poll_lock(m_threaded_poll);

  ServiceEntry *service = STLFindOrNull(m_services, key);
  if (service) {
    OLA_WARN << "Service " << key << " is already registered";
    avahi_threaded_poll_unlock(m_threaded_poll);
    return;
  } else {
    service = new ServiceEntry(service_name, type, port, options);
    STLInsertIfNotPresent(&m_services, key, service);
  }

  // If we're not running, then we'll register the service when the client
  // transitions to the running state. Otherwise register it now.
  if (m_client && avahi_client_get_state(m_client) == AVAHI_CLIENT_S_RUNNING) {
    InternalRegisterService(service);
  }
  avahi_threaded_poll_unlock(m_threaded_poll);
}


/*
 * This is a bit tricky because it can be called from either the main thread on
 * startup or from the poll thread.
 */
void AvahiDiscoveryAgent::ClientStateChanged(AvahiClientState state,
                                             AvahiClient *client) {
  // The first time this is called is from the avahi_client_new context. In
  // that case m_client is still null so we set it here.
  if (!m_client) {
    m_client = client;
  }

  OLA_INFO << "Client state changed to " << ClientStateToString(state);

  switch (state) {
    case AVAHI_CLIENT_S_RUNNING:
      // The server has startup successfully and registered its host
      // name on the network, so it's time to create our services.
      // register_stuff
      UpdateServices();
      break;
    case AVAHI_CLIENT_FAILURE:
      DeregisterAllServices();
      SetUpReconnectTimeout();
      break;
    case AVAHI_CLIENT_S_COLLISION:
      // There was a hostname collision on the network.
      // Let's drop our registered services. When the server is back
      // in AVAHI_SERVER_RUNNING state we will register them again with the
      // new host name.
      DeregisterAllServices();
      break;
    case AVAHI_CLIENT_S_REGISTERING:
      // The server records are now being established. This
      // might be caused by a host name change. We need to wait
      // for our own records to register until the host name is
      // properly established.
      DeregisterAllServices();
      break;
    case AVAHI_CLIENT_CONNECTING:
      break;
  }
}

void AvahiDiscoveryAgent::GroupStateChanged(const string &service_key,
                                            AvahiEntryGroup *group,
                                            AvahiEntryGroupState state) {
  OLA_INFO << "State for " << service_key << ", group " << group
           << " changed to " << GroupStateToString(state);

  ServiceEntry *service = STLFindOrNull(m_services, service_key);
  if (!service) {
    OLA_WARN << "Unknown service " << service_key << " changed to state "
             << state;
    return;
  }

  if (service->group != group) {
    if (service->group) {
      OLA_WARN << "Service group for " << service_key << " : "
               << service->group << " does not match callback group "
               << group;
    }
    return;
  }

  service->state = state;

  switch (state) {
    case AVAHI_ENTRY_GROUP_ESTABLISHED:
      break;
    case AVAHI_ENTRY_GROUP_COLLISION:
      RenameAndRegister(service);
      break;
    case AVAHI_ENTRY_GROUP_FAILURE:
      OLA_WARN << "Failed to register " << service_key
               << ": " << avahi_strerror(avahi_client_errno(m_client));
      break;
    case AVAHI_ENTRY_GROUP_UNCOMMITED:
    case AVAHI_ENTRY_GROUP_REGISTERING:
      break;
  }
}

void AvahiDiscoveryAgent::ReconnectTimeout() {
  if (m_client) {
    avahi_client_free(m_client);
    m_client = NULL;
  }
  CreateNewClient();
}

bool AvahiDiscoveryAgent::InternalRegisterService(ServiceEntry *service) {
  if (!service->params) {
    service->params = new EntryGroupParams(this, service->key());
  }

  if (!service->group) {
    service->group = avahi_entry_group_new(m_client, entry_callback,
                                           service->params);
    if (!service->group) {
      OLA_WARN << "avahi_entry_group_new() failed: "
               << avahi_client_errno(m_client);
      return false;
    }
  }

  if (!avahi_entry_group_is_empty(service->group)) {
    OLA_INFO << "Service group was not empty!";
    return false;
  }

  AvahiStringList *txt_args = NULL;
  RegisterOptions::TxtData::const_iterator iter = service->txt_data.begin();
  for (; iter != service->txt_data.end(); ++iter) {
    txt_args = avahi_string_list_add_pair(
        txt_args, iter->first.c_str(), iter->second.c_str());
  }

  // Populate the entry group
  int r = avahi_entry_group_add_service_strlst(
      service->group,
      service->if_index > 0 ? service->if_index : AVAHI_IF_UNSPEC,
      AVAHI_PROTO_INET, static_cast<AvahiPublishFlags>(0),
      service->actual_service_name.c_str(), service->type().c_str(), NULL, NULL,
      service->port, txt_args);

  avahi_string_list_free(txt_args);

  if (r) {
    if (r == AVAHI_ERR_COLLISION) {
      OLA_WARN << "Collision with local service!";
      return RenameAndRegister(service);
    }
    OLA_WARN << "avahi_entry_group_add_service failed: " << avahi_strerror(r);
    avahi_entry_group_reset(service->group);
    return false;
  }

  // Add any subtypes
  const vector<string> &sub_types = service->sub_types();
  if (!sub_types.empty()) {
    vector<string>::const_iterator iter = sub_types.begin();
    for (; iter != sub_types.end(); ++iter) {
      const string sub_type = *iter + "._sub." + service->type();
      OLA_INFO << "Adding " << sub_type;
      r = avahi_entry_group_add_service_subtype(
          service->group,
          service->if_index > 0 ? service->if_index : AVAHI_IF_UNSPEC,
          AVAHI_PROTO_INET, static_cast<AvahiPublishFlags>(0),
          service->actual_service_name.c_str(), service->type().c_str(), NULL,
          sub_type.c_str());
      if (r) {
        OLA_WARN << "Failed to add " << sub_type << ": " << avahi_strerror(r);
      }
    }
  }

  r = avahi_entry_group_commit(service->group);
  if (r) {
    avahi_entry_group_reset(service->group);
    OLA_WARN << "avahi_entry_group_commit failed: " << avahi_strerror(r);
    return false;
  }
  return true;
}

void AvahiDiscoveryAgent::CreateNewClient() {
  if (m_client) {
    OLA_WARN << "CreateNewClient called but m_client is not NULL";
    return;
  }

  if (m_threaded_poll) {
    int error;
    // In the successful case, m_client is set in the ClientStateChanged method
    m_client = avahi_client_new(avahi_threaded_poll_get(m_threaded_poll),
                                AVAHI_CLIENT_NO_FAIL, client_callback, this,
                                &error);
    if (m_client) {
      m_backoff.Reset();
    } else {
      OLA_WARN << "Failed to create Avahi client: " << avahi_strerror(error);
      SetUpReconnectTimeout();
    }
  }
}

void AvahiDiscoveryAgent::UpdateServices() {
  Services::iterator iter = m_services.begin();
  for (; iter != m_services.end(); ++iter) {
    if (iter->second->state == AVAHI_ENTRY_GROUP_UNCOMMITED) {
      InternalRegisterService(iter->second);
    }
  }
}

/**
 * De-register all services and clean up the AvahiEntryGroup and
 * EntryGroupParams data.
 */
void AvahiDiscoveryAgent::DeregisterAllServices() {
  Services::iterator iter = m_services.begin();
  for (; iter != m_services.end(); ++iter) {
    AvahiEntryGroup *group = iter->second->group;
    if (group) {
      avahi_entry_group_reset(group);
      avahi_entry_group_free(group);
      iter->second->group = NULL;
    }
    if (iter->second->params) {
      delete iter->second->params;
      iter->second->params = NULL;
    }
    iter->second->state = AVAHI_ENTRY_GROUP_UNCOMMITED;
  }
}

void AvahiDiscoveryAgent::SetUpReconnectTimeout() {
  // We don't strictly need an ExponentialBackoffPolicy here because the client
  // goes into the AVAHI_CLIENT_CONNECTING state if the server isn't running.
  // Still, it's a useful defense against spinning rapidly if something goes
  // wrong.
  TimeInterval delay = m_backoff.Next();
  OLA_INFO << "Re-creating avahi client in " << delay << "s";
  struct timeval tv;
  delay.AsTimeval(&tv);

  const AvahiPoll *poll = avahi_threaded_poll_get(m_threaded_poll);
  if (m_reconnect_timeout) {
    poll->timeout_update(m_reconnect_timeout, &tv);
  } else {
    m_reconnect_timeout = poll->timeout_new(
        avahi_threaded_poll_get(m_threaded_poll),
        &tv,
        reconnect_callback,
        this);
  }
}

bool AvahiDiscoveryAgent::RenameAndRegister(ServiceEntry *service) {
  char *new_name_str =
      avahi_alternative_service_name(service->actual_service_name.c_str());
  string new_name(new_name_str);
  avahi_free(new_name_str);

  OLA_WARN << "Service name collision for " << service->actual_service_name
           << " renaming to " << new_name;
  service->actual_service_name = new_name;
  return InternalRegisterService(service);
}

string AvahiDiscoveryAgent::ClientStateToString(AvahiClientState state) {
  switch (state) {
    case AVAHI_CLIENT_S_REGISTERING:
      return "AVAHI_CLIENT_S_REGISTERING";
    case AVAHI_CLIENT_S_RUNNING:
      return "AVAHI_CLIENT_S_RUNNING";
    case AVAHI_CLIENT_S_COLLISION:
      return "AVAHI_CLIENT_S_COLLISION";
    case AVAHI_CLIENT_FAILURE:
      return "AVAHI_CLIENT_FAILURE";
    case AVAHI_CLIENT_CONNECTING:
      return "AVAHI_CLIENT_CONNECTING";
    default:
      return "Unknown state";
  }
}

string AvahiDiscoveryAgent::GroupStateToString(AvahiEntryGroupState state) {
  switch (state) {
    case AVAHI_ENTRY_GROUP_UNCOMMITED:
      return "AVAHI_ENTRY_GROUP_UNCOMMITED";
    case AVAHI_ENTRY_GROUP_REGISTERING:
      return "AVAHI_ENTRY_GROUP_REGISTERING";
    case AVAHI_ENTRY_GROUP_ESTABLISHED:
      return "AVAHI_ENTRY_GROUP_ESTABLISHED";
    case AVAHI_ENTRY_GROUP_COLLISION:
      return "AVAHI_ENTRY_GROUP_COLLISION";
    case AVAHI_ENTRY_GROUP_FAILURE:
      return "AVAHI_ENTRY_GROUP_FAILURE";
    default:
      return "Unknown state";
  }
}
}  // namespace ola
