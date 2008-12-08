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

#include <llad/Universe.h>
#include <llad/logger.h>
#include <llad/Port.h>
#include "UniverseStore.h"
#include "Client.h"

#include <arpa/inet.h>
#include <iterator>
#include <string.h>
#include <stdio.h>

namespace lla {

/*
 * Create a new universe
 *
 * @param uid  the universe id of this universe
 */
Universe::Universe(int universe_id, UniverseStore *store) :
  m_universe_name(""),
  m_universe_id(universe_id),
  m_merge_mode(Universe::MERGE_LTP),
  m_universe_store(store),
  m_length(DMX_UNIVERSE_SIZE) {

  memset(m_data, 0x00, DMX_UNIVERSE_SIZE);
}


/*
 * Add a port to this universe
 *
 * @param prt  the port to add
 */
int Universe::AddPort(AbstractPort *port) {
  vector<AbstractPort*>::iterator iter;
  Universe *universe = port->GetUniverse();

  if (universe== this)
    return 0;

  // unpatch if required
  if (universe!= NULL) {
    Logger::instance()->log(Logger::DEBUG,
                            "Port %p is bound to universe %d",
                            port,
                            universe->UniverseId());
    universe->RemovePort(port);

    m_universe_store->DeleteUniverseIfInactive(universe);
  }

  // patch to this universe
  Logger::instance()->log(Logger::INFO,
                          "Patched %d to universe %d",
                          port->PortId() ,
                          m_universe_id);
  m_ports.push_back(port);
  port->SetUniverse(this);
  return 0;
}


/*
 * Remove a port from this universe. After calling this method you need to
 * check if this universe is still in use, and if not delete it
 *
 * @param prt the port to remove
 */
int Universe::RemovePort(AbstractPort *port) {
  vector<AbstractPort*>::iterator iter;

  iter = find(m_ports.begin(), m_ports.end(), port);

  if (iter != m_ports.end()) {
    m_ports.erase(iter);
    port->SetUniverse(NULL);
    Logger::instance()->log(Logger::DEBUG,
                            "Port %p has been removed from uni %d",
                            port,
                            m_universe_id);

  } else {
    Logger::instance()->log(Logger::DEBUG, "Could not find port in universe");
    return -1;
  }
  return 0;
}


/*
 * Add this client to this universe
 *
 * @param client the client to add
 */
int Universe::AddClient(Client *client) {
  vector<Client*>::iterator iter = find(m_clients.begin(),
                                        m_clients.end(),
                                        client);

  if (iter == m_clients.end())
    return 0;

  Logger::instance()->log(Logger::INFO,
                          "Added clientent %p to universe %d",
                          client,
                          m_universe_id);
  m_clients.push_back(client);
  return 0;
}


/*
 * Remove this client from the universe. After calling this method you need to
 * check if this universe is still in use, and if not delete it
 *
 * @param client  the client to remove
 */
int Universe::RemoveClient(Client *client) {
  vector<Client*>::iterator iter = find(m_clients.begin(),
                                        m_clients.end(),
                                        client);

  if (iter != m_clients.end()) {
    m_clients.erase(iter);
    Logger::instance()->log(Logger::INFO,
                            "Client %p has been removed from uni %d",
                            client,
                            m_universe_id);
  } else
    Logger::instance()->log(Logger::DEBUG, "Could not find client in universe");
  return 0;
}


/*
 * Set the dmx data for this universe
 *
 * @param  dmx  pointer to the dmx data
 * @param  len  the length of the dmx buffer
 */
int Universe::SetDMX(uint8_t *dmx, unsigned int length) {

  if (m_merge_mode == Universe::MERGE_LTP) {
    m_length = length < DMX_UNIVERSE_SIZE ? length : DMX_UNIVERSE_SIZE;
    memcpy(m_data, dmx, m_length);
  } else {
    // HTP, this is more difficult, we'll need a buffer per client
    // for now just set it
    //TODO: implement proper HTP merging from clients
    m_length = length < DMX_UNIVERSE_SIZE ? length : DMX_UNIVERSE_SIZE;
    memcpy(m_data, dmx, m_length);
  }
  return this->UpdateDependants();
}


/*
 * Get the dmx data for this universe
 *
 * @param dmx    the buffer to copy data into
 * @param length  the length of the buffer
 */
int Universe::GetDMX(uint8_t *dmx, unsigned int length) const {
  int len = length < m_length ? length : m_length;
  memcpy(dmx, m_data, len);
  return len;
}


/*
 * Get the dmx data for this universe
 *
 * @param length  the length of the buffer
 * @returns a pointer to the DMX data
 */
const uint8_t *Universe::GetDMX(int &length) const {
  length = m_length;
  return m_data;
}


/*
 * Call this when the dmx in a port that is part of this universe changes
 *
 * @param prt   the port that has changed
 */
int Universe::PortDataChanged(AbstractPort *port) {
  unsigned int i, len;

  if (m_merge_mode == Universe::MERGE_LTP) {
    // LTP merge mode
    // this is simple, find the port and copy the data
    for (i =0; i < m_ports.size(); i++) {
      if (m_ports[i] == port && port->CanRead()) {
        // read the new data and update our dependants
        m_length = port->ReadDMX(m_data, DMX_UNIVERSE_SIZE);
        UpdateDependants();
        break;
      }
    }
  } else {
    // htp merge mode
    // iterate over ports which we can read and take the highest value
    // of each channel
    bool first = true;
    for (i =0; i < m_ports.size(); i++) {
      if (port->CanRead()) {
        if (first) {
          m_length = port->ReadDMX(m_data, DMX_UNIVERSE_SIZE);
          first = false;
        } else {
          len = port->ReadDMX(m_merge, DMX_UNIVERSE_SIZE);
          Merge();
        }
      }
    }
    UpdateDependants();
  }

  return 0;
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
int Universe::UpdateDependants() {
  unsigned int i;

  // write to all ports assigned to this unviverse
  for (i=0; i < m_ports.size(); i++) {
    m_ports[i]->WriteDMX(m_data, m_length);
  }

  // write to all clients
  for (i=0; i < m_clients.size(); i++) {
    Logger::instance()->log(Logger::DEBUG, "Sending dmx data msg to client");
    m_clients[i]->SendDMX(m_universe_id, m_data, m_length);
  }
  return 0;
}


/*
 * HTP merge the merge buffer into the data buffer
 */
void Universe::Merge() {
  int i, l;

  // l is the length we merge over
  l = m_mlength < m_length ? m_mlength : m_length;

  for (i=0; i < l; i++) {
    m_data[i] = m_data[i] > m_merge[i] ? m_data[i] : m_merge[i];
  }

  if (m_mlength > m_length) {
    // copy the remaining over
    memcpy(&m_data[l], &m_merge[l], m_mlength - m_length);
    m_length = m_mlength;
  }
}

} //lla
