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
 * UniverseStore.cpp
 * The universe store class. This maintains the set of all active universes and
 * saves the settings.
 * Copyright (C) 2005-2008 Simon Newton
 */
#include <string>
#include <sstream>
#include <iostream>

#include <ola/ExportMap.h>
#include <olad/Universe.h>
#include <olad/Preferences.h>
#include "UniverseStore.h"

namespace ola {


UniverseStore::UniverseStore(class Preferences *preferences,
                             ExportMap *export_map):
  m_preferences(preferences),
  m_export_map(export_map) {

  if (export_map) {
    StringMap *map = export_map->GetStringMapVar(Universe::K_UNIVERSE_NAME_VAR,
        "name");
    map = export_map->GetStringMapVar(Universe::K_UNIVERSE_MODE_VAR, "mode");
    IntMap *int_map = export_map->GetIntMapVar(Universe::K_UNIVERSE_PORT_VAR,
        "count");
    int_map = export_map->GetIntMapVar(Universe::K_UNIVERSE_SOURCE_CLIENTS_VAR,
        "count");
    int_map = export_map->GetIntMapVar(Universe::K_UNIVERSE_SINK_CLIENTS_VAR,
        "count");
  }
}


/*
 * Cleanup
 */
UniverseStore::~UniverseStore() {
  DeleteAll();
}


/*
 * Lookup a universe from its universe_id
 * @param uid the uid of the required universe
 */
Universe *UniverseStore::GetUniverse(unsigned int universe_id) const {
  map<int, Universe*>::const_iterator iter;

  iter = m_universe_map.find(universe_id);
  if (iter != m_universe_map.end()) {
     return iter->second;
  }
  return NULL;
}


/*
 * Lookup a universe, or create it if it does not exist
 * @param uid the universe id
 * @return the universe, or NULL on error
 */
Universe *UniverseStore::GetUniverseOrCreate(unsigned int universe_id) {
  Universe *universe = GetUniverse(universe_id);

  if (!universe) {
    universe = new Universe(universe_id, this, m_export_map);

    if (universe) {
      pair<int, Universe*> pair(universe_id, universe);
      m_universe_map.insert(pair);

      if (m_preferences)
        RestoreUniverseSettings(universe);
    }
  }
  return universe;
}


/*
 * Returns a list of universes. This must be freed when you're
 * done with it.
 * @return a pointer to a vector of Universe*
 */
vector<Universe*> *UniverseStore::GetList() const {
  vector<Universe*> *list = new vector<Universe*>;
  list->reserve(UniverseCount());

  map<int ,Universe*>::const_iterator iter;
  for (iter = m_universe_map.begin(); iter != m_universe_map.end(); ++iter)
    list->push_back(iter->second);
  return list;
}


/*
 * Delete all universes
 */
int UniverseStore::DeleteAll() {
  map<int, Universe*>::iterator iter;

  for (iter = m_universe_map.begin(); iter != m_universe_map.end(); iter++) {
    SaveUniverseSettings(iter->second);
    delete iter->second;
  }
  m_deletion_candiates.clear();
  m_universe_map.clear();
  return 0;
}


/*
 * Mark a universe as a candiate for garbage collection.
 * @param universe the universe which has no clients or ports bound
 */
void UniverseStore::AddUniverseGarbageCollection(Universe *universe) {
  m_deletion_candiates.insert(universe);
}


/*
 * Check all the garbage collection candiates and delete the ones that aren't
 * needed.
 */
void UniverseStore::GarbageCollectUniverses() {
  set<Universe*>::iterator iter;
  map<int, Universe *>::iterator map_iter;

  for (iter = m_deletion_candiates.begin();
       iter != m_deletion_candiates.end(); iter++) {
    if (!(*iter)->IsActive()) {
      SaveUniverseSettings(*iter);
      m_universe_map.erase((*iter)->UniverseId());
      delete *iter;
    }
  }
  m_deletion_candiates.clear();
}


/*
 * Restore a universe's settings
 * @param uni  the universe to update
 */
bool UniverseStore::RestoreUniverseSettings(Universe *universe) const {
  string key, value;
  std::ostringstream oss;

  if (!universe)
    return 0;

  oss << std::dec << universe->UniverseId();

  // load name
  key = "uni_" + oss.str() + "_name";
  value = m_preferences->GetValue(key);

  if (!value.empty())
    universe->SetName(value);

  // load merge mode
  key = "uni_" + oss.str() + "_merge";
  value = m_preferences->GetValue(key);

  if (!value.empty()) {
    if (value == "HTP")
      universe->SetMergeMode(Universe::MERGE_HTP);
    else
      universe->SetMergeMode(Universe::MERGE_LTP);
  }
  return 0;
}


/*
 * Save this universe's settings.
 *
 * @param uni  the universe to save
 */
bool UniverseStore::SaveUniverseSettings(Universe *universe) {
  string key, mode;
  std::ostringstream oss;

  if (!universe || !m_preferences)
    return 0;

  oss << std::dec << universe->UniverseId();

  // save name
  key = "uni_" + oss.str() + "_name";
  m_preferences->SetValue(key, universe->Name());

  // save merge mode
  key = "uni_" + oss.str() + "_merge";
  mode = (universe->MergeMode() == Universe::MERGE_HTP ? "HTP" : "LTP");
  m_preferences->SetValue(key, mode);

  return 0;
}

} //ola
