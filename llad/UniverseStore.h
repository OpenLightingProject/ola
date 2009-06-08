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
 * UniverseStore.h
 * The Universe Store class - this manages the universes
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef UNIVERSE_STORE_H
#define UNIVERSE_STORE_H

#include <map>
#include <set>
#include <string>
#include <vector>

namespace lla {

class Universe;

class UniverseStore {
  public:
    UniverseStore(class Preferences *preferences, class ExportMap *export_map);
    ~UniverseStore() {};

    Universe *GetUniverse(unsigned int universe_id) const;
    Universe *GetUniverseOrCreate(unsigned int universe_id);

    int UniverseCount() const { return m_universe_map.size(); }
    std::vector<Universe*> *GetList() const;

    int DeleteAll();
    void AddUniverseGarbageCollection(Universe *universe);
    void GarbageCollectUniverses();

  private:
    UniverseStore(const lla::UniverseStore&);
    UniverseStore& operator=(const UniverseStore&);
    bool RestoreUniverseSettings(Universe *universe) const;
    bool SaveUniverseSettings(Universe *universe);

    Preferences *m_preferences;
    ExportMap *m_export_map;
    std::map<int, Universe*> m_universe_map;  // map of universe_id to Universe
    std::set<Universe*> m_deletion_candiates; // list of universes we may be
                                              // able to delete
};

} //lla
#endif
