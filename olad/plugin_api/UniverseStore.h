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
 * UniverseStore.h
 * The Universe Store class - this manages the universes
 * Copyright (C) 2005 Simon Newton
 */

#ifndef OLAD_PLUGIN_API_UNIVERSESTORE_H_
#define OLAD_PLUGIN_API_UNIVERSESTORE_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "ola/Clock.h"
#include "ola/base/Macro.h"

namespace ola {

class Universe;

/**
 * @brief Maintains a collection of Universe objects.
 */
class UniverseStore {
 public:
  /**
   * @brief Create a new UniverseStore.
   * @param preferences The Preferences store.
   * @param export_map the ExportMap to use for stats, may be NULL.
   */
  UniverseStore(class Preferences *preferences, class ExportMap *export_map);

  /**
   * @brief Destructor.
   */
  ~UniverseStore();

  /**
   * @brief Lookup a universe from its universe-id.
   * @param universe_id the universe-id of the universe.
   * @return the universe, or NULL if the universe doesn't exist.
   */
  Universe *GetUniverse(unsigned int universe_id) const;

  /**
   * @brief Lookup a universe, or create it if it does not exist.
   * @param universe_id the universe-id of the universe.
   * @return the universe, or NULL on error
   */
  Universe *GetUniverseOrCreate(unsigned int universe_id);

  /**
   * @brief Return the number of universes.
   */
  unsigned int UniverseCount() const { return m_universe_map.size(); }

  /**
   * @brief Returns a list of universes. This must be freed when you're
   * done with it.
   * @param[out] universes a pointer to a vector of Universes.
   */
  void GetList(std::vector<Universe*> *universes) const;

  /**
   * @brief Delete all universes.
   */
  void DeleteAll();

  /**
   * @brief Mark a universe as a candidate for garbage collection.
   * @param universe the Universe which has no clients or ports bound.
   */
  void AddUniverseGarbageCollection(Universe *universe);

  /**
   * @brief Garbage collect any pending universes.
   */
  void GarbageCollectUniverses();

 private:
  typedef std::map<unsigned int, Universe*> UniverseMap;

  Preferences *m_preferences;
  ExportMap *m_export_map;
  UniverseMap m_universe_map;
  std::set<Universe*> m_deletion_candidates;  // list of universes we may be
                                              // able to delete
  Clock m_clock;

  bool RestoreUniverseSettings(Universe *universe) const;
  bool SaveUniverseSettings(Universe *universe) const;

  static const unsigned int MINIMUM_RDM_DISCOVERY_INTERVAL;

  DISALLOW_COPY_AND_ASSIGN(UniverseStore);
};
}  // namespace ola
#endif  // OLAD_PLUGIN_API_UNIVERSESTORE_H_
