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
 * Represents a universe of DMX data
 * Copyright (C) 2005-2008 Simon Newton
 *
 */

#include <lla/Logging.h>
#include <llad/Port.h>
#include <llad/Universe.h>
#include "UniverseStore.h"
#include "Client.h"

#include <arpa/inet.h>
#include <iterator>
#include <algorithm>
#include <string.h>
#include <stdio.h>

namespace lla {

const string Universe::K_UNIVERSE_NAME_VAR = "universe_name";
const string Universe::K_UNIVERSE_MODE_VAR = "universe_mode";
const string Universe::K_UNIVERSE_PORT_VAR = "universe_ports";
const string Universe::K_UNIVERSE_CLIENTS_VAR = "universe_clients";
const string Universe::K_MERGE_HTP_STR = "htp";
const string Universe::K_MERGE_LTP_STR = "ltp";

/*
 * Create a new universe
 *
 * @param uid  the universe id of this universe
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
    m_export_map->GetIntMapVar(K_UNIVERSE_CLIENTS_VAR)->Set(m_universe_id_str, 0);
  }
}


/*
 * Delete this universe
 */
Universe::~Universe() {
  if (m_export_map) {
    m_export_map->GetStringMapVar(K_UNIVERSE_NAME_VAR)->Remove(m_universe_id_str);
    m_export_map->GetStringMapVar(K_UNIVERSE_MODE_VAR)->Remove(m_universe_id_str);
    m_export_map->GetIntMapVar(K_UNIVERSE_PORT_VAR)->Remove(m_universe_id_str);
    m_export_map->GetIntMapVar(K_UNIVERSE_CLIENTS_VAR)->Remove(m_universe_id_str);
  }
}

/*
 * Set the universe name
 */
void Universe::SetName(const string &name) {
  m_universe_name = name;
  UpdateName();
}


/*
 * Set the universe merge mode
 */
void Universe::SetMergeMode(merge_mode merge_mode) {
  m_merge_mode = merge_mode;
  UpdateMode();
}


/*
 * Add a port to this universe
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
 * Add this client to this universe
 * @param client the client to add
 */
bool Universe::AddClient(Client *client) {
  vector<Client*>::const_iterator iter = find(m_clients.begin(),
                                              m_clients.end(),
                                              client);

  if (iter != m_clients.end())
    return true;

  LLA_INFO << "Added client " << client << " to universe " << m_universe_id;
  m_clients.push_back(client);
  if (m_export_map) {
    IntMap *map = m_export_map->GetIntMapVar(K_UNIVERSE_CLIENTS_VAR);
    map->Set(m_universe_id_str, map->Get(m_universe_id_str) + 1);
  }
  return true;
}


/*
 * Remove this client from the universe. After calling this method you need to
 * check if this universe is still in use, and if not delete it
 * @param client  the client to remove
 */
bool Universe::RemoveClient(Client *client) {
  vector<Client*>::iterator iter = find(m_clients.begin(),
                                        m_clients.end(),
                                        client);

  if (iter != m_clients.end()) {
    m_clients.erase(iter);
    if (m_export_map) {
      IntMap *map = m_export_map->GetIntMapVar(K_UNIVERSE_CLIENTS_VAR);
      map->Set(m_universe_id_str, map->Get(m_universe_id_str) - 1);
    }
    LLA_INFO << "Client " << client << " has been removed from uni " <<
      m_universe_id;
  }

  if (!IsActive())
    m_universe_store->AddUniverseGarbageCollection(this);
  return true;
}


/*
 * Returns true if the client is bound to this universe
 * @param client the client to check for
 */
bool Universe::ContainsClient(class Client *client) const {
  vector<Client*>::const_iterator iter = find(m_clients.begin(),
                                              m_clients.end(),
                                              client);

  return iter != m_clients.end();
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
  vector<AbstractPort*>::const_iterator iter;

  if (m_merge_mode == Universe::MERGE_LTP) {
    // LTP merge mode
    for (iter = m_ports.begin(); iter != m_ports.end(); ++iter) {
      if (*iter == port && (*iter)->CanRead()) {
        const DmxBuffer &new_buffer = (*iter)->ReadDMX();
        if (new_buffer.Size())
          m_buffer = new_buffer;
      } else {
        LLA_INFO << "Trying to update a port which isn't bound to universe: "
          << UniverseId();
      }
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
bool Universe::ClientDataChanged(Client *client) {
  vector<Client*>::const_iterator iter;

  if (m_merge_mode == Universe::MERGE_LTP) {
    if (client) {
      const DmxBuffer &new_buffer = client->GetDMX(m_universe_id);
      if (new_buffer.Size())
        m_buffer = new_buffer;
    }
  } else {
    HTPMergeAllSources();
  }
  UpdateDependants();
  return true;
}


/*
 * Return true if this universe is in use
 */
bool Universe::IsActive() const {
  return m_ports.size() > 0 || m_clients.size() > 0;
}


// Private Methods
//-----------------------------------------------------------------------------


/*
 * Called when the dmx data for this universe changes,
 * updates everyone who needs to know (patched ports and network clients)
 *
 */
bool Universe::UpdateDependants() {
  vector<AbstractPort*>::const_iterator iter;
  vector<Client*>::const_iterator client_iter;

  // write to all ports assigned to this unviverse
  for (iter = m_ports.begin(); iter != m_ports.end(); ++iter) {
    (*iter)->WriteDMX(m_buffer);
  }

  // write to all clients
  for (client_iter = m_clients.begin(); client_iter != m_clients.end();
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
 * HTP Merge all sources (clients/ports)
 */
bool Universe::HTPMergeAllSources() {
  vector<AbstractPort*>::const_iterator iter;
  vector<Client*>::const_iterator client_iter;
  bool first = true;

  for (iter = m_ports.begin(); iter != m_ports.end(); ++iter) {
    if ((*iter)->CanRead()) {
      if (first) {
        m_buffer = (*iter)->ReadDMX();
        first = false;
      } else {
        m_buffer.HTPMerge((*iter)->ReadDMX());
      }
    }
  }

  // THIS IS WRONG
  // WRONG WRONG WRONG
  // clients are ones we send to - not recv from
  for (client_iter = m_clients.begin(); client_iter != m_clients.end();
       ++client_iter) {
    if (first) {
      m_buffer = (*client_iter)->GetDMX(m_universe_id);
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
