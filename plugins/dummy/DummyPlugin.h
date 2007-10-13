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
 * DummyPlugin.h
 * Interface for the dummyplugin class
 * Copyright (C) 2005-2007 Simon Newton
 */

#ifndef DUMMYPLUGIN_H
#define DUMMYPLUGIN_H

#include <llad/plugin.h>
#include <lla/plugin_id.h>

class DummyDevice;

class DummyPlugin : public Plugin {

  public:
    DummyPlugin(const PluginAdaptor *pa, lla_plugin_id id):
      Plugin(pa, id),
      m_dev(NULL) {}

    string get_name() const { return PLUGIN_NAME; }
    string get_desc() const;

  protected:
    string pref_suffix() const { return PLUGIN_PREFIX; }

  private:
    int start_hook();
    int stop_hook();
    int set_default_prefs();

    DummyDevice *m_dev ;    // the dummy device
    static const string PLUGIN_NAME;
    static const string PLUGIN_PREFIX;
};

#endif
