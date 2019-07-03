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

#include "olad/plugin_api/UniverseStore.h"

#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "ola/ExportMap.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/stl/STLUtils.h"
#include "olad/Preferences.h"
#include "olad/Universe.h"

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

    for (unsigned int i = 0; i < sizeof(vars) / sizeof(vars[0]); ++i) {
      export_map->GetUIntMapVar(string(vars[i]), "universe");
    }
  }
}

UniverseStore::~UniverseStore() {
  DeleteAll();
}

Universe *UniverseStore::GetUniverse(unsigned int universe_id) const {
  return STLFindOrNull(m_universe_map, universe_id);
}

Universe *UniverseStore::GetUniverseOrCreate(unsigned int universe_id) {
  UniverseMap::iterator iter = STLLookupOrInsertNull(
      &m_universe_map, universe_id);

  if (!iter->second) {
    iter->second = new Universe(universe_id, this, m_export_map, &m_clock);

    if (iter->second) {
      if (m_preferences) {
        RestoreUniverseSettings(iter->second);
      }
    } else {
      OLA_WARN << "Failed to create universe " << universe_id;
    }
  }
  return iter->second;
}

void UniverseStore::GetList(vector<Universe*> *universes) const {
  STLValues(m_universe_map, universes);
}

void UniverseStore::DeleteAll() {
  UniverseMap::iterator iter;

  for (iter = m_universe_map.begin(); iter != m_universe_map.end(); iter++) {
    SaveUniverseSettings(iter->second);
    delete iter->second;
  }
  m_deletion_candidates.clear();
  m_universe_map.clear();
}

void UniverseStore::AddUniverseGarbageCollection(Universe *universe) {
  m_deletion_candidates.insert(universe);
}

void UniverseStore::GarbageCollectUniverses() {
  set<Universe*>::iterator iter;
  UniverseMap::iterator map_iter;

  for (iter = m_deletion_candidates.begin();
       iter != m_deletion_candidates.end(); iter++) {
    if (!(*iter)->IsActive()) {
      SaveUniverseSettings(*iter);
      m_universe_map.erase((*iter)->UniverseId());
      delete *iter;
    }
  }
  m_deletion_candidates.clear();
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

  m_preferences->Save();

  return 0;
}
}  // namespace ola
