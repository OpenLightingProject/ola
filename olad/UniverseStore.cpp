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
 * UniverseStore.cpp
 * The universe store class. This maintains the set of all active universes and
 * saves the settings.
 * Copyright (C) 2005 Simon Newton
 */

#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>


#include "ola/ExportMap.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "olad/Preferences.h"
#include "olad/Universe.h"
#include "olad/UniverseStore.h"
namespace ola {

using std::pair;
using std::set;
using std::string;
using std::vector;

const unsigned int UniverseStore::MINIMUM_RDM_DISCOVERY_INTERVAL = 30;

UniverseStore::UniverseStore(Preferences *preferences,
                             ExportMap *export_map)
    : m_preferences(preferences),
      m_export_map(export_map) {
  if (export_map) {
    export_map->GetStringMapVar(Universe::K_UNIVERSE_NAME_VAR, "universe");
    export_map->GetStringMapVar(Universe::K_UNIVERSE_MODE_VAR, "universe");

    const char *vars[] = {
      Universe::K_FPS_VAR,
      Universe::K_UNIVERSE_INPUT_PORT_VAR,
      Universe::K_UNIVERSE_OUTPUT_PORT_VAR,
      Universe::K_UNIVERSE_SINK_CLIENTS_VAR,
      Universe::K_UNIVERSE_SOURCE_CLIENTS_VAR,
      Universe::K_UNIVERSE_UID_COUNT_VAR,
    };

    for (unsigned int i = 0; i < sizeof(vars) / sizeof(vars[0]); ++i)
      export_map->GetUIntMapVar(string(vars[i]), "universe");
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
  universe_map::const_iterator iter = m_universe_map.find(universe_id);
  if (iter != m_universe_map.end())
     return iter->second;
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
    universe = new Universe(universe_id, this, m_export_map, &m_clock);

    if (universe) {
      pair<unsigned int, Universe*> pair(universe_id, universe);
      m_universe_map.insert(pair);

      if (m_preferences)
        RestoreUniverseSettings(universe);
    } else {
      OLA_WARN << "Failed to create universe " << universe_id;
    }
  }
  return universe;
}


/*
 * Returns a list of universes. This must be freed when you're
 * done with it.
 * @return a pointer to a vector of Universe*
 */
void UniverseStore::GetList(vector<Universe*> *universes) const {
  universes->reserve(UniverseCount());

  universe_map::const_iterator iter;
  for (iter = m_universe_map.begin(); iter != m_universe_map.end(); ++iter)
    universes->push_back(iter->second);
}


/*
 * Delete all universes
 */
void UniverseStore::DeleteAll() {
  universe_map::iterator iter;

  for (iter = m_universe_map.begin(); iter != m_universe_map.end(); iter++) {
    SaveUniverseSettings(iter->second);
    delete iter->second;
  }
  m_deletion_candiates.clear();
  m_universe_map.clear();
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
  universe_map::iterator map_iter;

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

  // load RDM discovery interval
  key = "uni_" + oss.str() + "_rdm_discovery_interval";
  value = m_preferences->GetValue(key);

  if (!value.empty()) {
    unsigned int interval;
    if (StringToInt(value, &interval, true)) {
      if (interval != 0 && interval < MINIMUM_RDM_DISCOVERY_INTERVAL) {
        OLA_WARN << "RDM Discovery interval for universe " <<
          universe->UniverseId() << " less than the minimum of " <<
          MINIMUM_RDM_DISCOVERY_INTERVAL;
        interval = MINIMUM_RDM_DISCOVERY_INTERVAL;
      }
      OLA_DEBUG << "RDM Discovery interval for " << oss.str() << " is " <<
        interval;
      TimeInterval discovery_interval(interval, 0);
      universe->SetRDMDiscoveryInterval(discovery_interval);
    } else {
      OLA_WARN << "Invalid RDM discovery interval for universe " <<
        universe->UniverseId() << ", value was " << value;
    }
  }
  return 0;
}


/*
 * Save this universe's settings.
 * @param universe, the universe to save
 */
bool UniverseStore::SaveUniverseSettings(Universe *universe) const {
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

  // We don't save the RDM Discovery interval since it can only be set in the
  // config files for now.

  return 0;
}
}  // namespace ola
