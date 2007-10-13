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
 * sandnetplugin.h
 * Interface for the sandnet plugin class
 * Copyright (C) 2005-2006  Simon Newton
 */

#ifndef SANDNETPLUGIN_H
#define SANDNETPLUGIN_H

#include <llad/plugin.h>
#include <lla/plugin_id.h>

class SandNetDevice;

class SandNetPlugin : public Plugin {

  public:
    SandNetPlugin(const PluginAdaptor *pa, lla_plugin_id id) :
      Plugin(pa, id),
      m_dev(NULL) {}

    int test(void *data);
    string get_name() const   { return PLUGIN_NAME; }
    string get_desc() const;

  protected:
    string pref_suffix() const { return PLUGIN_PREFIX; }

  private:
    int start_hook();
    int stop_hook();
    int set_default_prefs();
    SandNetDevice *m_dev;    // only have one device

    static const string SANDNET_NODE_NAME;
    static const string SANDNET_DEVICE_NAME;
    static const string PLUGIN_NAME;
    static const string PLUGIN_PREFIX;

};

#endif

