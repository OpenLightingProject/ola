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
 * ArtNetPlugin.h
 * Interface for the artnet plugin class
 * Copyright (C) 2005-2007 Simon Newton
 */

#ifndef ARTNETPLUGIN_H
#define ARTNETPLUGIN_H

#include <string>
#include <llad/plugin.h>
#include <lla/plugin_id.h>

using namespace std;

class ArtNetDevice;

class ArtNetPlugin : public Plugin {

  public:
    ArtNetPlugin(const PluginAdaptor *pa, lla_plugin_id id):
      Plugin(pa,id),
      m_dev(NULL) {}

    ~ArtNetPlugin() {}

    string get_name() const { return PLUGIN_NAME; }
    string get_desc() const;

  protected:
    string pref_suffix() const { return PLUGIN_PREFIX; }

  private:
    int start_hook();
    int stop_hook();
    int set_default_prefs();
    ArtNetDevice *m_dev; // only have one device

    static const int ARTNET_SUBNET;
    static const string ARTNET_LONG_NAME;
    static const string ARTNET_SHORT_NAME;
    static const string PLUGIN_NAME;
    static const string PLUGIN_PREFIX;
};

#endif

