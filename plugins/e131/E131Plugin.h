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
 * E131Plugin.h
 * Interface for the E1.131 plugin class
 * Copyright (C) 2007 Simon Newton
 */

#ifndef E131PLUGIN_H
#define E131PLUGIN_H

#include <string>
#include <llad/plugin.h>
#include <lla/plugin_id.h>

using namespace std;

class E131Plugin : public Plugin {

  public:
    E131Plugin(const PluginAdaptor *pa, lla_plugin_id id):
      Plugin(pa,id),
      m_dev(NULL) {}

    ~E131Plugin() {}

    string get_name() const { return PLUGIN_NAME; }
    string get_desc() const;
    string pref_suffix() const { return PLUGIN_PREFIX; }

  private:
    int start_hook();
    int stop_hook();
    class E131Device *m_dev; // only have one device
    class NetServer *m_ns;

    static const string PLUGIN_NAME;
    static const string PLUGIN_PREFIX;
};

#endif

