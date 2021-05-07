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
 * Universe.cpp
 * Represents a universe of DMX data.
 * Copyright (C) 2005 Simon Newton
 *
 * Each universe has the following:
 *   A human readable name
 *   A DmxBuffer with the current dmx data
 *   A MergeMode, either LTP (latest takes precedence) or HTP (highest takes
 *     precedence)
 *   A list of ports bound to this universe. If a port is an input, we use the
 *     data to update the DmxBuffer according to the MergeMode. If a port is an
 *     output, we notify the port whenever the DmxBuffer changes.
 *   A list of source clients. which provide us with data for updating the
 *     DmxBuffer per the merge mode.
 *   A list of sink clients, which we update whenever the DmxBuffer changes.
 */

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ola/base/Array.h"
#include "ola/Logging.h"
#include "ola/MultiCallback.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/stl/STLUtils.h"
#include "ola/strings/Format.h"
#include "olad/Port.h"
#include "olad/Universe.h"
#include "olad/plugin_api/Client.h"
#include "olad/plugin_api/UniverseStore.h"

namespace ola {

using ola::rdm::RDMDiscoveryCallback;
using ola::rdm::RDMReply;
using ola::rdm::RDMRequest;
using ola::rdm::RunRDMCallback;
using ola::rdm::UID;
using ola::strings::ToHex;
using std::auto_ptr;
using std::map;
using std::ostringstream;
using std::set;
using std::string;
using std::vector;

const char Universe::K_UNIVERSE_UID_COUNT_VAR[] = "universe-uids";
const char Universe::K_FPS_VAR[] = "universe-dmx-frames";
const char Universe::K_MERGE_HTP_STR[] = "htp";
const char Universe::K_MERGE_LTP_STR[] = "ltp";
const char Universe::K_UNIVERSE_INPUT_PORT_VAR[] = "universe-input-ports";
const char Universe::K_UNIVERSE_MODE_VAR[] = "universe-mode";
const char Universe::K_UNIVERSE_NAME_VAR[] = "universe-name";
const char Universe::K_UNIVERSE_OUTPUT_PORT_VAR[] = "universe-output-ports";
const char Universe::K_UNIVERSE_RDM_REQUESTS[] = "universe-rdm-requests";
const char Universe::K_UNIVERSE_SINK_CLIENTS_VAR[] = "universe-sink-clients";
const char Universe::K_UNIVERSE_SOURCE_CLIENTS_VAR[] =
    "universe-source-clients";

/*
 * Create a new universe
 * @param uid  the universe id of this universe
 * @param store the store this universe came from
 * @param export_map the ExportMap that we update
 */
Universe::Universe(unsigned int universe_id, UniverseStore *store,
                   ExportMap *export_map,
                   Clock *clock)
    : m_universe_name(""),
      m_universe_id(universe_id),
      m_active_priority(ola::dmx::SOURCE_PRIORITY_MIN),
      m_merge_mode(Universe::MERGE_LTP),
      m_universe_store(store),
      m_export_map(export_map),
      m_clock(clock),
      m_rdm_discovery_interval(),
      m_last_discovery_time(),
      m_transaction_number_sequence() {
  ostringstream universe_id_str, universe_name_str;
  universe_id_str << universe_id;
  m_universe_id_str = universe_id_str.str();
  universe_name_str << "Universe " << universe_id;
  m_universe_name = universe_name_str.str();

  UpdateName();
  UpdateMode();

  const char *vars[] = {
    K_FPS_VAR,
    K_UNIVERSE_INPUT_PORT_VAR,
    K_UNIVERSE_OUTPUT_PORT_VAR,
    K_UNIVERSE_RDM_REQUESTS,
    K_UNIVERSE_SINK_CLIENTS_VAR,
    K_UNIVERSE_SOURCE_CLIENTS_VAR,
    K_UNIVERSE_UID_COUNT_VAR,
  };

  if (m_export_map) {
    for (unsigned int i = 0; i < arraysize(vars); ++i) {
      (*m_export_map->GetUIntMapVar(vars[i]))[m_universe_id_str] = 0;
    }
  }

  // We set the last discovery time to now, since most ports will trigger
  // discovery when they are patched.
  clock->CurrentMonotonicTime(&m_last_discovery_time);
}


/*
 * Delete this universe
 */
Universe::~Universe() {
  const char *string_vars[] = {
    K_UNIVERSE_NAME_VAR,
    K_UNIVERSE_MODE_VAR,
  };

  const char *uint_vars[] = {
    K_FPS_VAR,
    K_UNIVERSE_INPUT_PORT_VAR,
    K_UNIVERSE_OUTPUT_PORT_VAR,
    K_UNIVERSE_RDM_REQUESTS,
    K_UNIVERSE_SINK_CLIENTS_VAR,
    K_UNIVERSE_SOURCE_CLIENTS_VAR,
    K_UNIVERSE_UID_COUNT_VAR,
  };

  if (m_export_map) {
    for (unsigned int i = 0; i < arraysize(string_vars); ++i) {
      m_export_map->GetStringMapVar(string_vars[i])->Remove(m_universe_id_str);
    }
    for (unsigned int i = 0; i < arraysize(uint_vars); ++i) {
      m_export_map->GetUIntMapVar(uint_vars[i])->Remove(m_universe_id_str);
    }
  }
}


/*
 * Set the universe name
 * @param name the new universe name
 */
void Universe::SetName(const string &name) {
  m_universe_name = name;
  UpdateName();

  // notify ports
  vector<OutputPort*>::const_iterator iter;
  for (iter = m_output_ports.begin(); iter != m_output_ports.end(); ++iter) {
    (*iter)->UniverseNameChanged(name);
  }
}


/*
 * Set the universe merge mode
 * @param merge_mode the new merge_mode
 */
void Universe::SetMergeMode(enum merge_mode merge_mode) {
  m_merge_mode = merge_mode;
  UpdateMode();
}


/*
 * Add an InputPort to this universe.
 * @param port the port to add
 */
bool Universe::AddPort(InputPort *port) {
  return GenericAddPort(port, &m_input_ports);
}


/*
 * Add an OutputPort to this universe.
 * @param port the port to add
 */
bool Universe::AddPort(OutputPort *port) {
  return GenericAddPort(port, &m_output_ports);
}


/*
 * Remove a port from this universe.
 * @param port the port to remove
 * @return true if the port was removed, false if it didn't exist
 */
bool Universe::RemovePort(InputPort *port) {
  return GenericRemovePort(port, &m_input_ports);
}


/*
 * Remove a port from this universe.
 * @param port the port to remove
 * @return true if the port was removed, false if it didn't exist
 */
bool Universe::RemovePort(OutputPort *port) {
  bool ret = GenericRemovePort(port, &m_output_ports, &m_output_uids);

  if (m_export_map) {
    (*m_export_map->GetUIntMapVar(K_UNIVERSE_UID_COUNT_VAR))[m_universe_id_str]
        = m_output_uids.size();
  }
  return ret;
}


/*
 * Check if this port is bound to this universe
 * @param port the port to check for
 * @return true if the port exists in this universe, false otherwise
 */
bool Universe::ContainsPort(InputPort *port) const {
  return GenericContainsPort(port, m_input_ports);
}


/*
 * Check if this port is bound to this universe
 * @param port the port to check for
 * @return true if the port exists in this universe, false otherwise
 */
bool Universe::ContainsPort(OutputPort *port) const {
  return GenericContainsPort(port, m_output_ports);
}


/*
 * Get a list of input ports associated with this universe
 * @param ports, the vector to be populated
 */
void Universe::InputPorts(vector<InputPort*> *ports) const {
  ports->clear();
  std::copy(m_input_ports.begin(), m_input_ports.end(),
            std::back_inserter(*ports));
}


/*
 * Get a list of output ports associated with this universe
 * @param ports, the vector to be populated
 */
void Universe::OutputPorts(vector<OutputPort*> *ports) const {
  ports->clear();
  std::copy(m_output_ports.begin(), m_output_ports.end(),
            std::back_inserter(*ports));
}


/*
 * @brief Add a client as a source for this universe
 * @param client the client to add
 * @return true
 */
bool Universe::AddSourceClient(Client *client) {
  // Check to see if it exists already. It doesn't make sense to have multiple
  //  clients
  if (STLReplace(&m_source_clients, client, false)) {
    return true;
  }

  OLA_INFO << "Added source client, " << client << " to universe "
           << m_universe_id;

  SafeIncrement(K_UNIVERSE_SOURCE_CLIENTS_VAR);
  return true;
}


/*
 * @param Remove this client from the universe.
 * @note After the client is removed we internally check if this universe is
 * still in use, and if not delete it
 * @param client the client to remove
 * @return true is this client was removed, false if it didn't exist
 */
bool Universe::RemoveSourceClient(Client *client) {
  if (!STLRemove(&m_source_clients, client)) {
    return false;
  }

  SafeDecrement(K_UNIVERSE_SOURCE_CLIENTS_VAR);

  OLA_INFO << "Source client " << client << " has been removed from uni "
           << m_universe_id;

  if (!IsActive()) {
    m_universe_store->AddUniverseGarbageCollection(this);
  }
  return true;
}


/*
 * Check if this universe contains a client as a source
 * @param client the client to check for
 * @returns true if this universe contains the client, false otherwise
 */
bool Universe::ContainsSourceClient(Client *client) const {
  return STLContains(m_source_clients, client);
}


/*
 * @brief Add a client as a sink for this universe
 * @param client the client to add
 * @return true if client was added, and false if it was already a sink client
 */
bool Universe::AddSinkClient(Client *client) {
  if (!STLInsertIfNotPresent(&m_sink_clients, client)) {
    return false;
  }

  OLA_INFO << "Added sink client, " << client << " to universe "
           << m_universe_id;

  SafeIncrement(K_UNIVERSE_SINK_CLIENTS_VAR);
  return true;
}


/*
 * @param Remove this sink client from the universe.
 * @note After the client is removed we internally check if this universe is
 * still in use, and if not delete it
 * @param client the client to remove
 * @return true is this client was removed, false if it didn't exist
 */
bool Universe::RemoveSinkClient(Client *client) {
  if (!STLRemove(&m_sink_clients, client)) {
    return false;
  }

  SafeDecrement(K_UNIVERSE_SINK_CLIENTS_VAR);

  OLA_INFO << "Sink client " << client << " has been removed from uni "
           << m_universe_id;

  if (!IsActive()) {
    m_universe_store->AddUniverseGarbageCollection(this);
  }
  return true;
}

/*
 * Check if this universe contains a client as a sink
 * @param client the client to check for
 * @returns true if this universe contains the client, false otherwise
 */
bool Universe::ContainsSinkClient(Client *client) const {
  return STLContains(m_sink_clients, client);
}


/*
 * Set the dmx data for this universe, this overrides anything from the clients
 * or ports but will be overridden in the next update.
 * @param buffer the dmx buffer with the data
 * @return true is we updated all ports/clients, false otherwise
 */
bool Universe::SetDMX(const DmxBuffer &buffer) {
  if (!buffer.Size()) {
    OLA_INFO << "Trying to SetDMX with a 0 length dmx buffer, universe "
             << UniverseId();
    return true;
  }
  m_buffer.Set(buffer);
  return UpdateDependants();
}


/*
 * Call this when the dmx in a port that is part of this universe changes
 * @param port the port that has changed
 */
bool Universe::PortDataChanged(InputPort *port) {
  if (!ContainsPort(port)) {
    OLA_INFO << "Trying to update a port which isn't bound to universe: "
             << UniverseId();
    return false;
  }
  if (MergeAll(port, NULL)) {
    UpdateDependants();
  }
  return true;
}


/*
 * Called to indicate that data from a client has changed
 */
bool Universe::SourceClientDataChanged(Client *client) {
  if (!client) {
    return false;
  }

  AddSourceClient(client);   // always add since this may be the first call
  if (MergeAll(NULL, client)) {
    UpdateDependants();
  }
  return true;
}


/**
 * @brief Clean old source clients
 */
void Universe::CleanStaleSourceClients() {
  SourceClientMap::iterator iter = m_source_clients.begin();
  while (iter != m_source_clients.end()) {
    if (iter->second) {
      // if stale remove it
      m_source_clients.erase(iter++);
      SafeDecrement(K_UNIVERSE_SOURCE_CLIENTS_VAR);
      OLA_INFO << "Removed Stale Client";
      if (!IsActive()) {
        m_universe_store->AddUniverseGarbageCollection(this);
      }
    } else {
      // clear the stale flag
      iter->second = true;
      ++iter;
    }
  }
}


/*
 * Handle a RDM request for this universe, ownership of the request object is
 * transferred to this method.
 */
void Universe::SendRDMRequest(RDMRequest *request_ptr,
                              ola::rdm::RDMCallback *callback) {
  auto_ptr<RDMRequest> request(request_ptr);

  OLA_INFO << "Universe " << UniverseId() << ", RDM request to "
           << request->DestinationUID() << ", SD: " << request->SubDevice()
           << ", CC " << ToHex(request->CommandClass()) << ", TN "
           << static_cast<int>(request->TransactionNumber()) << ", PID "
           << ToHex(request->ParamId()) << ", PDL: "
           << request->ParamDataSize();

  SafeIncrement(K_UNIVERSE_RDM_REQUESTS);

  if (request->DestinationUID().IsBroadcast()) {
    if (m_output_ports.empty()) {
      RunRDMCallback(
          callback,
          request->IsDUB() ? ola::rdm::RDM_TIMEOUT :
              ola::rdm::RDM_WAS_BROADCAST);
      return;
    }

    // send this request to all ports
    broadcast_request_tracker *tracker = new broadcast_request_tracker;
    tracker->expected_count = m_output_ports.size();
    tracker->current_count = 0;
    tracker->status_code = (request->IsDUB() ?
        ola::rdm::RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED :
        ola::rdm::RDM_WAS_BROADCAST);
    tracker->callback = callback;
    vector<OutputPort*>::iterator port_iter;

    for (port_iter = m_output_ports.begin(); port_iter != m_output_ports.end();
         ++port_iter) {
      // because each port deletes the request, we need to copy it here
      if (request->IsDUB()) {
        (*port_iter)->SendRDMRequest(
            request->Duplicate(),
            NewSingleCallback(this,
                              &Universe::HandleBroadcastDiscovery,
                              tracker));
      } else  {
        (*port_iter)->SendRDMRequest(
            request->Duplicate(),
            NewSingleCallback(this, &Universe::HandleBroadcastAck, tracker));
      }
    }
  } else {
    map<UID, OutputPort*>::iterator iter =
        m_output_uids.find(request->DestinationUID());

    if (iter == m_output_uids.end()) {
      OLA_WARN << "Can't find UID " << request->DestinationUID()
               << " in the output universe map, dropping request";
      RunRDMCallback(callback, ola::rdm::RDM_UNKNOWN_UID);
    } else {
      iter->second->SendRDMRequest(request.release(), callback);
    }
  }
}


/*
 * Trigger RDM discovery for this universe
 */
void Universe::RunRDMDiscovery(RDMDiscoveryCallback *on_complete, bool full) {
  if (full) {
    OLA_INFO << "Full RDM discovery triggered for universe " << m_universe_id;
  } else {
    OLA_INFO << "Incremental RDM discovery triggered for universe "
             << m_universe_id;
  }

  m_clock->CurrentMonotonicTime(&m_last_discovery_time);

  // we need to make a copy of the ports first, because the callback may run at
  // any time so we need to guard against the port list changing.
  vector<OutputPort*> output_ports(m_output_ports.size());
  copy(m_output_ports.begin(), m_output_ports.end(), output_ports.begin());

  // the multicallback that indicates when discovery is done
  BaseCallback0<void> *discovery_complete = NewMultiCallback(
      output_ports.size(),
      NewSingleCallback(this, &Universe::DiscoveryComplete, on_complete));

  // Send Discovery requests to all ports, as each of these return they'll
  // update the UID map. When all ports callbacks have run, the MultiCallback
  // will trigger, running the DiscoveryCallback.
  vector<OutputPort*>::iterator iter;
  for (iter = output_ports.begin(); iter != output_ports.end(); ++iter) {
    if (full) {
      (*iter)->RunFullDiscovery(
          NewSingleCallback(this,
                            &Universe::PortDiscoveryComplete,
                            discovery_complete,
                            *iter));
    } else {
      (*iter)->RunIncrementalDiscovery(
          NewSingleCallback(this,
                            &Universe::PortDiscoveryComplete,
                            discovery_complete,
                            *iter));
    }
  }
}


/*
 * Update the UID : port mapping with this new data
 */
void Universe::NewUIDList(OutputPort *port, const ola::rdm::UIDSet &uids) {
  map<UID, OutputPort*>::iterator iter = m_output_uids.begin();
  while (iter != m_output_uids.end()) {
    if (iter->second == port && !uids.Contains(iter->first)) {
      m_output_uids.erase(iter++);
    } else {
      ++iter;
    }
  }

  ola::rdm::UIDSet::Iterator set_iter = uids.Begin();
  for (; set_iter != uids.End(); ++set_iter) {
    iter = m_output_uids.find(*set_iter);
    if (iter == m_output_uids.end()) {
      m_output_uids[*set_iter] = port;
    } else if (iter->second != port) {
      OLA_WARN << "UID " << *set_iter << " seen on more than one port";
    }
  }

  if (m_export_map) {
    (*m_export_map->GetUIntMapVar(K_UNIVERSE_UID_COUNT_VAR))[m_universe_id_str]
        = m_output_uids.size();
  }
}


/*
 * Returns the complete UIDSet for this universe
 */
void Universe::GetUIDs(ola::rdm::UIDSet *uids) const {
  map<UID, OutputPort*>::const_iterator iter = m_output_uids.begin();
  for (; iter != m_output_uids.end(); ++iter) {
    uids->AddUID(iter->first);
  }
}


/**
 * Return the number of uids in the universe
 */
unsigned int Universe::UIDCount() const {
  return m_output_uids.size();
}

/**
 * Return the RDM transaction number to use
 */
uint8_t Universe::GetRDMTransactionNumber() {
  return m_transaction_number_sequence.Next();
}

/*
 * Return true if this universe is in use (has at least one port or client).
 */
bool Universe::IsActive() const {
  // any of the following means the port is active
  return !(m_output_ports.empty() && m_input_ports.empty() &&
           m_source_clients.empty() && m_sink_clients.empty());
}


// Private Methods
//-----------------------------------------------------------------------------


/*
 * Called when the dmx data for this universe changes,
 * updates everyone who needs to know (patched ports and network clients)
 */
bool Universe::UpdateDependants() {
  vector<OutputPort*>::const_iterator iter;
  set<Client*>::const_iterator client_iter;

  // write to all ports assigned to this universe
  for (iter = m_output_ports.begin(); iter != m_output_ports.end(); ++iter) {
    (*iter)->WriteDMX(m_buffer, m_active_priority);
  }

  // write to all clients
  for (client_iter = m_sink_clients.begin();
       client_iter != m_sink_clients.end();
       ++client_iter) {
    (*client_iter)->SendDMX(m_universe_id, m_active_priority, m_buffer);
  }

  SafeIncrement(K_FPS_VAR);
  return true;
}


/*
 * Update the name in the export map.
 */
void Universe::UpdateName() {
  if (!m_export_map) {
    return;
  }
  StringMap *name_map = m_export_map->GetStringMapVar(K_UNIVERSE_NAME_VAR);
  (*name_map)[m_universe_id_str] = m_universe_name;
}


/*
 * Update the mode in the export map.
 */
void Universe::UpdateMode() {
  if (!m_export_map) {
    return;
  }
  StringMap *mode_map = m_export_map->GetStringMapVar(K_UNIVERSE_MODE_VAR);
  (*mode_map)[m_universe_id_str] = (m_merge_mode == Universe::MERGE_LTP ?
                                    K_MERGE_LTP_STR : K_MERGE_HTP_STR);
}


/*
 * HTP Merge all sources (clients/ports)
 * @pre sources.size >= 2
 * @param sources the list of DmxSources to merge
 */
void Universe::HTPMergeSources(const vector<DmxSource> &sources) {
  vector<DmxSource>::const_iterator iter;
  m_buffer.Reset();

  for (iter = sources.begin(); iter != sources.end(); ++iter) {
    m_buffer.HTPMerge(iter->Data());
  }
}


/*
 * Merge all port/client sources.
 * This does a priority based merge as documented at:
 * https://wiki.openlighting.org/index.php/OLA_Merging_Algorithms
 * @param port the input port that changed or NULL
 * @param client the client that changed or NULL
 * @returns true if the data for this universe changed, false otherwise
 */
bool Universe::MergeAll(const InputPort *port, const Client *client) {
  vector<DmxSource> active_sources;

  vector<InputPort*>::const_iterator iter;
  SourceClientMap::const_iterator client_iter;

  m_active_priority = ola::dmx::SOURCE_PRIORITY_MIN;
  TimeStamp now;
  m_clock->CurrentMonotonicTime(&now);
  bool changed_source_is_active = false;

  // Find the highest active ports
  for (iter = m_input_ports.begin(); iter != m_input_ports.end(); ++iter) {
    DmxSource source = (*iter)->SourceData();
    if (!source.IsSet() || !source.IsActive(now) || !source.Data().Size()) {
      continue;
    }

    if (source.Priority() > m_active_priority) {
      changed_source_is_active = false;
      active_sources.clear();
      m_active_priority = source.Priority();
    }

    if (source.Priority() == m_active_priority) {
      active_sources.push_back(source);
      if (*iter == port) {
        changed_source_is_active = true;
      }
    }
  }

  // find the highest priority active clients
  for (client_iter = m_source_clients.begin();
       client_iter != m_source_clients.end();
       ++client_iter) {
    const DmxSource &source = client_iter->first->SourceData(UniverseId());

    if (!source.IsSet() || !source.IsActive(now) || !source.Data().Size()) {
      continue;
    }

    if (source.Priority() > m_active_priority) {
      changed_source_is_active = false;
      active_sources.clear();
      m_active_priority = source.Priority();
    }

    if (source.Priority() == m_active_priority) {
      active_sources.push_back(source);
      if (client_iter->first == client) {
        changed_source_is_active = true;
      }
    }
  }

  if (active_sources.empty()) {
    OLA_WARN << "Something changed but we didn't find any active sources "
             << " for universe " << UniverseId();
    return false;
  }

  if (!changed_source_is_active) {
    // this source didn't have any effect, skip
    return false;
  }

  // only one source at the active priority
  if (active_sources.size() == 1) {
    m_buffer.Set(active_sources[0].Data());
  } else {
    // multi source merge
    if (m_merge_mode == Universe::MERGE_LTP) {
      vector<DmxSource>::const_iterator source_iter = active_sources.begin();
      DmxSource changed_source;
      if (port) {
        changed_source = port->SourceData();
      } else {
        changed_source = client->SourceData(UniverseId());
      }

      // check that the current port/client is newer than all other active
      // sources
      for (; source_iter != active_sources.end(); source_iter++) {
        if (changed_source.Timestamp() < source_iter->Timestamp()) {
          return false;
        }
      }
      // if we made it to here this is the newest source
      m_buffer.Set(changed_source.Data());
    } else {
      HTPMergeSources(active_sources);
    }
  }
  return true;
}


/**
 * Called when discovery completes on a single ports.
 */
void Universe::PortDiscoveryComplete(BaseCallback0<void> *on_complete,
                                     OutputPort *output_port,
                                     const ola::rdm::UIDSet &uids) {
  NewUIDList(output_port, uids);
  on_complete->Run();
}


/**
 * Called when discovery completes on all ports.
 */
void Universe::DiscoveryComplete(RDMDiscoveryCallback *on_complete) {
  ola::rdm::UIDSet uids;
  GetUIDs(&uids);
  if (on_complete) {
    on_complete->Run(uids);
  }
}


/**
 * Track fan-out responses for a broadcast request.
 * This increments the port counter until we reach the expected value, and
 * which point we run the callback for the client.
 */
void Universe::HandleBroadcastAck(broadcast_request_tracker *tracker,
                                  RDMReply *reply) {
  tracker->current_count++;
  if (reply->StatusCode() != ola::rdm::RDM_WAS_BROADCAST) {
    // propagate errors though
    tracker->status_code = reply->StatusCode();
  }

  if (tracker->current_count == tracker->expected_count) {
    // all ports have completed
    RunRDMCallback(tracker->callback, tracker->status_code);
    delete tracker;
  }
}


/**
 * Handle the DUB responses. This is unique because unlike an RDM splitter can
 * can return the DUB responses from each port (485 line in the splitter
 * world). We do this by concatenating the vectors together.
 *
 * The response codes should be one of:
 *   RDM_DUB_RESPONSE - got a DUB response
 *   RDM_TIMEOUT - no response received
 *   RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED - the port doesn't support DUB
 *
 * The above list is ordered in highest to lowest precedence, i.e. if we get
 * any port with a RDM_DUB_RESPONSE, this overrides any other message.
 */
void Universe::HandleBroadcastDiscovery(
    broadcast_request_tracker *tracker,
    RDMReply *reply) {
  tracker->current_count++;

  if (reply->StatusCode() == ola::rdm::RDM_DUB_RESPONSE) {
    // RDM_DUB_RESPONSE is the highest priority
    tracker->status_code = ola::rdm::RDM_DUB_RESPONSE;
  } else if (reply->StatusCode() == ola::rdm::RDM_TIMEOUT &&
             tracker->status_code != ola::rdm::RDM_DUB_RESPONSE) {
    // RDM_TIMEOUT is the second highest
    tracker->status_code = reply->StatusCode();
  } else if (tracker->status_code != ola::rdm::RDM_DUB_RESPONSE &&
             tracker->status_code != ola::rdm::RDM_TIMEOUT) {
    // everything else follows
    tracker->status_code = reply->StatusCode();
  }

  // append any packets to the main packets vector
  tracker->frames.insert(tracker->frames.end(),
                         reply->Frames().begin(),
                         reply->Frames().end());

  if (tracker->current_count == tracker->expected_count) {
    // all ports have completed
    RDMReply reply(tracker->status_code, NULL, tracker->frames);
    tracker->callback->Run(&reply);
    delete tracker;
  }
}


/*
 * Helper function to increment an Export Map variable
 */
void Universe::SafeIncrement(const string &name) {
  if (m_export_map) {
    (*m_export_map->GetUIntMapVar(name))[m_universe_id_str]++;
  }
}

/*
 * Helper function to decrement an Export Map variable
 */
void Universe::SafeDecrement(const string &name) {
  if (m_export_map) {
    (*m_export_map->GetUIntMapVar(name))[m_universe_id_str]--;
  }
}


/*
 * Add an Input or Output port to this universe.
 * @param port, the port to add
 * @param ports, the vector of ports to add to
 */
template<class PortClass>
bool Universe::GenericAddPort(PortClass *port, vector<PortClass*> *ports) {
  if (find(ports->begin(), ports->end(), port) != ports->end()) {
    return true;
  }

  ports->push_back(port);
  if (m_export_map) {
    UIntMap *map = m_export_map->GetUIntMapVar(
        IsInputPort<PortClass>() ? K_UNIVERSE_INPUT_PORT_VAR :
        K_UNIVERSE_OUTPUT_PORT_VAR);
    (*map)[m_universe_id_str]++;
  }
  return true;
}


/*
 * Remove an Input or Output port from this universe.
 * @param port, the port to add
 * @param ports, the vector of ports to remove from
 */
template<class PortClass>
bool Universe::GenericRemovePort(PortClass *port,
                                 vector<PortClass*> *ports,
                                 map<UID, PortClass*> *uid_map) {
  typename vector<PortClass*>::iterator iter =
    find(ports->begin(), ports->end(), port);

  if (iter == ports->end()) {
    OLA_DEBUG << "Could not find port " << port->UniqueId() << " in universe "
      << UniverseId();
    return true;
  }

  ports->erase(iter);
  if (m_export_map) {
    UIntMap *map = m_export_map->GetUIntMapVar(
        IsInputPort<PortClass>() ? K_UNIVERSE_INPUT_PORT_VAR :
        K_UNIVERSE_OUTPUT_PORT_VAR);
    (*map)[m_universe_id_str]--;
  }

  if (!IsActive()) {
    m_universe_store->AddUniverseGarbageCollection(this);
  }

  // Remove any uids that mapped to this port
  if (uid_map) {
    typename map<UID, PortClass*>::iterator uid_iter = uid_map->begin();
    while (uid_iter != uid_map->end()) {
      if (uid_iter->second == port) {
        uid_map->erase(uid_iter++);
      } else {
        ++uid_iter;
      }
    }
  }
  return true;
}


/*
 * Check if this universe contains a particular port.
 * @param port, the port to add
 * @param ports, the vector of ports to remove from
 */
template<class PortClass>
bool Universe::GenericContainsPort(PortClass *port,
                                   const vector<PortClass*> &ports) const {
  return find(ports.begin(), ports.end(), port) != ports.end();
}
}  // namespace  ola
