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
 * universemanager.h
 * the universe manager class
 * Copyright (C) 2005-2006  Simon Newton
 */

#ifndef UNIVERSE_MANAGER_H
#define UNIVERSE_MANAGER_H

#include <string>

#include <llad/preferences.h>

class UniverseStore {
  public:

    UniverseStore() : m_prefs("universes") {};
    ~UniverseStore() {};

    int load();
    int save();

    int store_uni(Universe *uni);
    int retrieve_uni(Universe *uni);

  private:
    Preferences m_prefs;


};

#endif
