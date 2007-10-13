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
 *
 * shownetplugin.h
 * Interface for the shownet plugin class
 * Copyright (C) 2005  Simon Newton
 */

#ifndef SHOWNETPLUGIN_H
#define SHOWNETPLUGIN_H

#include <llad/plugin.h>
#include <lla/plugin_id.h>

class ShowNetDevice;

class ShowNetPlugin : public Plugin {

  public:
    ShowNetPlugin(const PluginAdaptor *pa, lla_plugin_id id) :
      Plugin(pa, id),
      m_prefs(NULL),
      m_dev(NULL),
      m_enabled(false) {}

    int start();
    int stop();
    bool is_enabled() const       { return m_enabled; }
    string get_name() const       { return "ShowNet Plugin"; }
    string get_desc() const;

  private:
    int load_prefs();

    class Preferences *m_prefs;
    ShowNetDevice *m_dev;    // only have one device
    bool m_enabled;      // are we running
};

#endif

