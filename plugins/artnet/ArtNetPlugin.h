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
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef ARTNETPLUGIN_H
#define ARTNETPLUGIN_H

#include <string>
#include <llad/Plugin.h>
#include <lla/plugin_id.h>

namespace lla {
namespace plugin {

using lla::Plugin;
using lla::PluginAdaptor;
using namespace std;

class ArtNetDevice;

class ArtNetPlugin : public Plugin {
  public:
    ArtNetPlugin(const PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor),
      m_device(NULL) {}

    ~ArtNetPlugin() {}

    string Name() const { return PLUGIN_NAME; }
    lla_plugin_id Id() const { return LLA_PLUGIN_ARTNET; }
    string Description() const;

  protected:
    string PreferencesSuffix() const { return PLUGIN_PREFIX; }

  private:
    bool StartHook();
    bool StopHook();
    int SetDefaultPreferences();

    ArtNetDevice *m_device; // only have one device

    static const string ARTNET_SUBNET;
    static const string ARTNET_LONG_NAME;
    static const string ARTNET_SHORT_NAME;
    static const string PLUGIN_NAME;
    static const string PLUGIN_PREFIX;
};

} //plugin
} //lla
#endif

