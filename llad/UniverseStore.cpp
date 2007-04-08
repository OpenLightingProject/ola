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
 * IniverseStore.cpp
 * The universe store class. This enabled universes to save their settings
 * Copyright (C) 2005-2006  Simon Newton
 */

#include <string>
#include <sstream>
#include <iostream>

#include <llad/universe.h>
#include "UniverseStore.h"


/*
 * Load from disk
 */
int UniverseStore::load() {
  return m_prefs.load();
}


/*
 * Save all settings to disk
 */
int UniverseStore::save() {
  return m_prefs.save();
}


/*
 * Save this universe's settings in the store
 *
 * @param uni  the universe to save
 */
int UniverseStore::store_uni(Universe *uni) {
  string key, mode;
  std::ostringstream oss;

  if(uni != NULL) {
    oss << std::dec << uni->get_uid();

    // save name
    key = "uni_" + oss.str() + "_name";
    m_prefs.set_val(key, uni->get_name());

    // save merge mode
    key = "uni_" + oss.str() + "_merge";
    mode = (uni->get_merge_mode() == Universe::MERGE_HTP ? "HTP" : "LTP");
    m_prefs.set_val(key, mode);
  }

  return 0;
}


/*
 * Restore a universe's settings
 *
 * @param uni  the universe to update
 */
int UniverseStore::retrieve_uni(Universe *uni) {
  string key, val;
  std::ostringstream oss;

  if(uni != NULL) {
    oss << std::dec << uni->get_uid();

    // load name
    key = "uni_" + oss.str() + "_name";
    val = m_prefs.get_val(key);

    if (val != "")
      uni->set_name(val, false);

    // load merge mode
    key = "uni_" + oss.str() + "_merge";
    val = m_prefs.get_val(key);

    if (val != "") {
      if ( val == "HTP")
        uni->set_merge_mode(Universe::MERGE_HTP, false);
      else
        uni->set_merge_mode(Universe::MERGE_LTP, false);
    }
  }
  return 0;
}
