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
 * EspNetPlugin.h
 * Interface for the espnet plugin class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef ESPNETPLUGIN_H
#define ESPNETPLUGIN_H

#include <olad/Plugin.h>
#include <olad/PluginAdaptor.h>
#include <ola/plugin_id.h>

namespace ola {
namespace espnet {

class EspNetDevice;

class EspNetPlugin: public ola::Plugin {
  public:
    EspNetPlugin(const ola::PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor),
      m_device(NULL) {}

    string Name() const { return PLUGIN_NAME; }
    string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_ESPNET; }
    string PluginPrefix() const { return PLUGIN_PREFIX; }

  private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();

    EspNetDevice *m_device;
    static const string ESPNET_NODE_NAME;
    static const string ESPNET_DEVICE_NAME;
    static const string PLUGIN_NAME;
    static const string PLUGIN_PREFIX;
};

} //espnet
} //ola

#endif

