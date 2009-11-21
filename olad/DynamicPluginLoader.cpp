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
 * DynamicPluginLoader.cpp
 * This class is responsible for loading and unloading the plugins
 * Copyright (C) 2005-2007 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <vector>
#include "olad/Plugin.h"
#include "olad/DynamicPluginLoader.h"

#include "plugins/dummy/DummyPlugin.h"
#include "plugins/espnet/EspNetPlugin.h"
#include "plugins/opendmx/OpenDmxPlugin.h"
#include "plugins/sandnet/SandnetPlugin.h"
#include "plugins/shownet/ShowNetPlugin.h"
#include "plugins/stageprofi/StageProfiPlugin.h"
#include "plugins/usbpro/UsbProPlugin.h"

#ifdef HAVE_ARTNET
#include "plugins/artnet/ArtNetPlugin.h"
#endif

/*
#ifdef HAVE_PATHPORT
#include "plugins/pathport/PathportPlugin.h"
#endif
*/

#ifdef HAVE_DMX4LINUX
#include "plugins/dmx4linux/Dmx4LinuxPlugin.h"
#endif


namespace ola {

using std::vector;

/*
 * Load the plugins that we were linked against
 *
 * @return   0 on sucess, -1 on failure
 */
int DynamicPluginLoader::LoadPlugins() {
  AbstractPlugin *plugin;

  plugin = new ola::plugin::DummyPlugin(m_plugin_adaptor);
  m_plugins.push_back(plugin);
  plugin = new ola::plugin::OpenDmxPlugin(m_plugin_adaptor);
  m_plugins.push_back(plugin);
  plugin = new ola::plugin::StageProfiPlugin(m_plugin_adaptor);
  m_plugins.push_back(plugin);
  plugin = new ola::plugin::UsbProPlugin(m_plugin_adaptor);
  m_plugins.push_back(plugin);
  plugin = new ola::plugin::EspNetPlugin(m_plugin_adaptor);
  m_plugins.push_back(plugin);
  plug = new SandNetPlugin(m_pa, OLA_PLUGIN_SANDNET);
  m_plugins.push_back(plug);
  plug = new ShowNetPlugin(m_pa, OLA_PLUGIN_SHOWNET);
  m_plugins.push_back(plug);

#ifdef HAVE_ARTNET
  plugin = new ola::plugin::ArtNetPlugin(m_plugin_adaptor);
  m_plugins.push_back(plugin);
#endif

  /*
#ifdef HAVE_PATHPORT
  plug = new PathportPlugin(m_pa, OLA_PLUGIN_PATHPORT);
  m_plugins.push_back(plug);
#endif
*/

#ifdef HAVE_DMX4LINUX
  plugin = new ola::plugin::Dmx4LinuxPlugin(m_plugin_adaptor);
  m_plugins.push_back(plugin);
#endif

  return 0;
}


/*
 * Unload all plugins, this also stops them if they aren't already.
 */
int DynamicPluginLoader::UnloadPlugins() {
  vector<AbstractPlugin*>::iterator iter;

  for (iter = m_plugins.begin(); iter != m_plugins.end(); ++iter) {
    if ((*iter)->IsEnabled())
      (*iter)->Stop();
    delete *iter;
  }
  m_plugins.clear();
  return 0;
}


/*
 * Return the number of plugins loaded
 *
 * @return the number of plugins loaded
 */
int DynamicPluginLoader::PluginCount() const {
  return m_plugins.size();
}


/*
 * Returns a list of plugins
 */
vector<class AbstractPlugin*> DynamicPluginLoader::Plugins() const {
  return m_plugins;
}


/*
 * Return the plugin with the specified id.
 * @param plugin_id the id of the plugin
 * @return the plugin corresponding to this id or NULL if not found
 */
AbstractPlugin* DynamicPluginLoader::GetPlugin(ola_plugin_id plugin_id) const {
  vector<AbstractPlugin*>::const_iterator iter;

  for (iter = m_plugins.begin(); iter != m_plugins.end(); ++iter)
    if ((*iter)->Id() == plugin_id)
      return *iter;
  return NULL;
}
}  // ola
