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
 * Universe.cpp
 * Represents a universe of DMX data.
 * Copyright (C) 2005-2009 Simon Newton
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

#include <lla/Logging.h>
#include <llad/Port.h>
#include <llad/Universe.h>
#include "UniverseStore.h"
#include "Client.h"

#include <iterator>
#include <algorithm>

namespace lla {

const string Universe::K_UNIVERSE_NAME_VAR = "universe_name";
const string Universe::K_UNIVERSE_MODE_VAR = "universe_mode";
const string Universe::K_UNIVERSE_PORT_VAR = "universe_ports";
const string Universe::K_UNIVERSE_SOURCE_CLIENTS_VAR =
    "universe_source_clients";
const string Universe::K_UNIVERSE_SINK_CLIENTS_VAR = "universe_sink_clients";
const string Universe::K_MERGE_HTP_STR = "htp";
const string Universe::K_MERGE_LTP_STR = "ltp";

/*
 * Create a new universe
 * @param uid  the universe id of this universe
 * @param store the store this universe came from
 * @param export_map the ExportMap that we update
 */
Universe::Universe(unsigned int universe_id, UniverseStore *store,
                   ExportMap *export_map):
  m_universe_name(""),
  m_universe_id(universe_id),
  m_merge_mode(Universe::MERGE_LTP),
  m_universe_store(store),
  m_export_map(export_map) {

  stringstream universe_id_str;
  universe_id_str << universe_id;
  m_universe_id_str = universe_id_str.str();

  UpdateName();
  UpdateMode();

  if (m_export_map) {
    m_export_map->GetIntMapVar(K_UNIVERSE_PORT_VAR)->Set(m_universe_id_str, 0);
    m_export_map->GetIntMapVar(K_UNIVERSE_SOURCE_CLIENTS_VAR)->Set(
        m_universe_id_str, 0);
    m_export_map->GetIntMapVar(K_UNIVERSE_SINK_CLIENTS_VAR)->Set(
        m_universe_id_str, 0);
  }
}


/*
 * Delete this universe
 */
Universe::~Universe() {
  if (m_export_map) {
    m_export_map->GetStringMapVar(K_UNIVERSE_NAME_VAR)->Remove(
        m_universe_id_str);
    m_export_map->GetStringMapVar(K_UNIVERSE_MODE_VAR)->Remove(
        m_universe_id_str);
    m_export_map->GetIntMapVar(K_UNIVERSE_PORT_VAR)->Remove(m_universe_id_str);
    m_export_map->GetIntMapVar(K_UNIVERSE_SOURCE_CLIENTS_VAR)->Remove(
        m_universe_id_str);
    m_export_map->GetIntMapVar(K_UNIVERSE_SINK_CLIENTS_VAR)->Remove(
        m_universe_id_str);
  }
}

/*
 * Set the universe name
 * @param name the new universe name
 */
void Universe::SetName(const string &name) {
  m_universe_name = name;
  UpdateName();
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
 * Add a port to this universe. If we CanRead() on this port is true, it'll be
 * used as a source for DMX data. If CanWrite() on this port is true we'll
 * update this port when we get new DMX data.
 * @param port the port to add
 */
bool Universe::AddPort(AbstractPort *port) {
  vector<AbstractPort*>::iterator iter;
  Universe *universe = port->GetUniverse();

  if (universe == this)
    return true;

  // unpatch if required
  if (universe) {
    LLA_DEBUG << "Port " << port->PortId() << " is bound to universe " <<
      universe->UniverseId();
    universe->RemovePort(port);
  }

  // patch to this universe
  LLA_INFO << "Patched " << port->PortId() << " to universe " << m_universe_id;
  m_ports.push_back(port);
  port->SetUniverse(this);
  if (m_export_map) {
    IntMap *map = m_export_map->GetIntMapVar(K_UNIVERSE_PORT_VAR);
    map->Set(m_universe_id_str, map->Get(m_universe_id_str) + 1);
  }
  return true;
}


/*
 * Remove a port from this universe.
 * @param port the port to remove
 * @return true if the port was removed, false if it didn't exist
 */
bool Universe::RemovePort(AbstractPort *port) {
  vector<AbstractPort*>::iterator iter;
  iter = find(m_ports.begin(), m_ports.end(), port);

  if (iter != m_ports.end()) {
    m_ports.erase(iter);
    port->SetUniverse(NULL);
    if (m_export_map) {
      IntMap *map = m_export_map->GetIntMapVar(K_UNIVERSE_PORT_VAR);
      map->Set(m_universe_id_str, map->Get(m_universe_id_str) - 1);
    }
    LLA_DEBUG << "Port " << port << " has been removed from uni " <<
      m_universe_id;
  } else {
    LLA_DEBUG << "Could not find port in universe";
    return false;
  }

  if (!IsActive())
    m_universe_store->AddUniverseGarbageCollection(this);
  return true;
}


/*
 * Check if this port is bound to this universe
 * @param port the port to check for
 * @return true if the port exists in this universe, false otherwise
 */
bool Universe::ContainsPort(AbstractPort *port) const {
  vector<AbstractPort*>::const_iterator iter =
      find(m_ports.begin(), m_ports.end(), port);

  return iter != m_ports.end();
}


/*
 * Add a client as a source for this universe
 * @param client the client to add
 */
bool Universe::AddSourceClient(Client *client) {
  if (ContainsSourceClient(client))
    return false;
  return AddClient(client, true);
}


/*
 * Remove a client as a source for this universe
 * @param client the client to remove
 */
bool Universe::RemoveSourceClient(Client *client) {
  return RemoveClient(client, true);
}


/*
 * Check if this universe contains a client as a source
 * @param client the client to check for
 * @returns true if this universe contains the client, false otherwise
 */
bool Universe::ContainsSourceClient(Client *client) const {
  set<Client*>::const_iterator iter = find(m_source_clients.begin(),
                                           m_source_clients.end(),
                                           client);

  return iter != m_source_clients.end();
}


/*
 * Add a client as a sink for this universe
 * @param client the client to add
 */
bool Universe::AddSinkClient(Client *client) {
  if (ContainsSinkClient(client))
    return false;
  return AddClient(client, false);
}


/*
 * Remove a client as a sink for this universe
 * @param client the client to remove
 */
bool Universe::RemoveSinkClient(Client *client) {
  return RemoveClient(client, false);
}

/*
 * Check if this universe contains a client as a sink
 * @param client the client to check for
 * @returns true if this universe contains the client, false otherwise
 */
bool Universe::ContainsSinkClient(Client *client) const {
  set<Client*>::const_iterator iter = find(m_sink_clients.begin(),
                                           m_sink_clients.end(),
                                           client);

  return iter != m_sink_clients.end();
}


/*
 * Set the dmx data for this universe
 * @param buffer the dmx buffer with the data
 * @return true is we updated all ports/clients, false otherwise
 */
bool Universe::SetDMX(const DmxBuffer &buffer) {
  if (!buffer.Size()) {
    LLA_INFO << "Trying to SetDMX with a 0 length dmx buffer, universe " <<
      UniverseId();
    return true;
  }

  if (m_merge_mode == Universe::MERGE_LTP) {
    m_buffer = buffer;
  } else {
    HTPMergeAllSources();
    m_buffer.HTPMerge(buffer);
  }
  return this->UpdateDependants();
}


/*
 * Call this when the dmx in a port that is part of this universe changes
 * @param port the port that has changed
 */
bool Universe::PortDataChanged(AbstractPort *port) {
  if (!ContainsPort(port)) {
    LLA_INFO << "Trying to update a port which isn't bound to universe: "
      << UniverseId();
    return false;
  }

  if (m_merge_mode == Universe::MERGE_LTP) {
    // LTP merge mode
    if (port->CanRead()) {
      const DmxBuffer &new_buffer = port->ReadDMX();
      if (new_buffer.Size())
        m_buffer = new_buffer;
    }
  } else {
    // htp merge mode
    HTPMergeAllSources();
  }
  UpdateDependants();
  return true;
}


/*
 * Called to indicate that data from a client has changed
 */
bool Universe::SourceClientDataChanged(Client *client) {
  if (!client)
    return false;

  if (m_merge_mode == Universe::MERGE_LTP) {
    const DmxBuffer &new_buffer = client->GetDMX(m_universe_id);
    if (new_buffer.Size())
      m_buffer = new_buffer;
  } else {
    // add the client if we're in HTP mode
    if (!ContainsSourceClient(client)) {
      AddSourceClient(client);
    }
    HTPMergeAllSources();
  }
  UpdateDependants();
  return true;
}


/*
 * Return true if this universe is in use (has at least one port or client).
 */
bool Universe::IsActive() const {
  return m_ports.size() || m_source_clients.size() || m_sink_clients.size();
}


// Private Methods
//-----------------------------------------------------------------------------


/*
 * Called when the dmx data for this universe changes,
 * updates everyone who needs to know (patched ports and network clients)
 */
bool Universe::UpdateDependants() {
  vector<AbstractPort*>::const_iterator iter;
  set<Client*>::const_iterator client_iter;

  // write to all ports assigned to this unviverse
  for (iter = m_ports.begin(); iter != m_ports.end(); ++iter) {
    (*iter)->WriteDMX(m_buffer);
  }

  // write to all clients
  for (client_iter = m_sink_clients.begin();
       client_iter != m_sink_clients.end();
       ++client_iter) {
    LLA_DEBUG << "Sending dmx data msg to client";
    (*client_iter)->SendDMX(m_universe_id, m_buffer);
  }
  return true;
}


/*
 * Update the name in the export map.
 */
void Universe::UpdateName() {
  if (!m_export_map)
    return;
  StringMap *name_map = m_export_map->GetStringMapVar(K_UNIVERSE_NAME_VAR);
  name_map->Set(m_universe_id_str, m_universe_name);
}


/*
 * Update the mode in the export map.
 */
void Universe::UpdateMode() {
  if (!m_export_map)
    return;
  StringMap *mode_map = m_export_map->GetStringMapVar(K_UNIVERSE_MODE_VAR);
  mode_map->Set(m_universe_id_str,
                m_merge_mode == Universe::MERGE_LTP ?
                K_MERGE_LTP_STR : K_MERGE_HTP_STR);
}


/*
 * Add this client to this universe
 * @param client the client to add
 * @pre the client doesn't already exist in the set
 */
bool Universe::AddClient(Client *client, bool is_source) {
  set<Client*> &clients = is_source ? m_source_clients : m_sink_clients;
  clients.insert(client);
  LLA_INFO << "Added " << (is_source ? "source" : "sink") << " client, " <<
    client << " to universe " << m_universe_id;

  if (m_export_map) {
    const string &map_name = is_source ? K_UNIVERSE_SOURCE_CLIENTS_VAR :
      K_UNIVERSE_SINK_CLIENTS_VAR;
    IntMap *map = m_export_map->GetIntMapVar(map_name);
    map->Set(m_universe_id_str, map->Get(m_universe_id_str) + 1);
  }
  return true;
}


/*
 * Remove this client from the universe. After calling this method you need to
 * check if this universe is still in use, and if not delete it
 * @param client the client to remove
 * @return true is this client was removed, false if it didn't exist
 */
bool Universe::RemoveClient(Client *client, bool is_source) {
  set<Client*> &clients = is_source ? m_source_clients : m_sink_clients;
  set<Client*>::iterator iter = find(clients.begin(),
                                     clients.end(),
                                     client);

  if (iter == clients.end())
    return false;

  clients.erase(iter);
  if (m_export_map) {
    const string &map_name = is_source ? K_UNIVERSE_SOURCE_CLIENTS_VAR :
      K_UNIVERSE_SINK_CLIENTS_VAR;
    IntMap *map = m_export_map->GetIntMapVar(map_name);
    map->Set(m_universe_id_str, map->Get(m_universe_id_str) - 1);
  }
  LLA_INFO << "Client " << client << " has been removed from uni " <<
    m_universe_id;

  if (!IsActive())
    m_universe_store->AddUniverseGarbageCollection(this);
  return true;
}



/*
 * HTP Merge all sources (clients/ports)
 */
bool Universe::HTPMergeAllSources() {
  vector<AbstractPort*>::const_iterator iter;
  set<Client*>::const_iterator client_iter;
  bool first = true;

  for (iter = m_ports.begin(); iter != m_ports.end(); ++iter) {
    if ((*iter)->CanRead()) {
      if (first) {
        // We do a copy here to avoid a delete/new operation later
        m_buffer.Set((*iter)->ReadDMX());
        first = false;
      } else {
        m_buffer.HTPMerge((*iter)->ReadDMX());
      }
    }
  }

  for (client_iter = m_source_clients.begin();
       client_iter != m_source_clients.end();
       ++client_iter) {
    if (first) {
      // We do a copy here to avoid a delete/new operation later
      m_buffer.Set((*client_iter)->GetDMX(m_universe_id));
      first = false;
    } else {
      m_buffer.HTPMerge((*client_iter)->GetDMX(m_universe_id));
    }
  }

  // we have no data yet, just reset the buffer
  if (first) {
    m_buffer.Reset();
  }
  return true;
}


} //lla
