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

#include <llad/logger.h>

#include "DynamicPluginLoader.h"

#include "plugins/dummy/dummyplugin.h"
#include "plugins/opendmx/opendmxplugin.h"
#include "plugins/usbpro/usbproplugin.h"
#include "plugins/stageprofi/stageprofiplugin.h"

#ifdef HAVE_ARTNET
#include "plugins/artnet/ArtNetPlugin.h"
#endif

#ifdef HAVE_ESPNET
#include "plugins/espnet/espnetplugin.h"
#endif

#ifdef HAVE_PATHPORT
#include "plugins/pathport/PathportPlugin.h"
#endif

#ifdef HAVE_SANDNET
#include "plugins/sandnet/SandnetPlugin.h"
#endif

#ifdef HAVE_SHOWNET
#include "plugins/shownet/shownetplugin.h"
#endif

#ifdef HAVE_DMX4LINUX
#include "plugins/dmx4linux/Dmx4LinuxPlugin.h"
#endif


#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <stdio.h>


/*
 * Load the plugins that we were linked against
 *
 * @return   0 on sucess, -1 on failure
 */
int DynamicPluginLoader::load_plugins() {
  Plugin *plug;

  plug = new DummyPlugin(m_pa, LLA_PLUGIN_DUMMY);
  m_plugin_vect.push_back(plug);
  plug = new OpenDmxPlugin(m_pa, LLA_PLUGIN_OPENDMX);
  m_plugin_vect.push_back(plug);
  plug = new StageProfiPlugin(m_pa, LLA_PLUGIN_STAGEPROFI);
  m_plugin_vect.push_back(plug);
  plug = new UsbProPlugin(m_pa, LLA_PLUGIN_USBPRO);
  m_plugin_vect.push_back(plug);

#ifdef HAVE_ARTNET
  plug = new ArtNetPlugin(m_pa, LLA_PLUGIN_ARTNET);
  m_plugin_vect.push_back(plug);
#endif

#ifdef HAVE_ESPNET
  plug = new EspNetPlugin(m_pa, LLA_PLUGIN_ESPNET);
  m_plugin_vect.push_back(plug);
#endif

#ifdef HAVE_PATHPORT
  plug = new PathportPlugin(m_pa, LLA_PLUGIN_PATHPORT);
  m_plugin_vect.push_back(plug);
#endif

#ifdef HAVE_SANDNET
  plug = new SandNetPlugin(m_pa, LLA_PLUGIN_SANDNET);
  m_plugin_vect.push_back(plug);
#endif

#ifdef HAVE_SHOWNET
  plug = new ShowNetPlugin(m_pa, LLA_PLUGIN_SHOWNET);
  m_plugin_vect.push_back(plug);
#endif

#ifdef HAVE_DMX4LINUX
  plug = new Dmx4LinuxPlugin(m_pa, LLA_PLUGIN_DMX4LINUX);
  m_plugin_vect.push_back(plug);
#endif

  return 0;
}


/*
 * unload all plugins
 *
 */
int DynamicPluginLoader::unload_plugins() {
  unsigned int i;
  for (i=0; i < m_plugin_vect.size(); i++) {
    if (m_plugin_vect[i]->is_enabled()) {
      m_plugin_vect[i]->stop();
    }
    delete m_plugin_vect[i];
  }
  m_plugin_vect.clear();
  return 0;
}


/*
 * Return the number of plugins loaded
 *
 * @return the number of plugins loaded
 */
int DynamicPluginLoader::plugin_count() const {
  return m_plugin_vect.size();
}


/*
 * Return the plugin with the specified id
 *
 * @param id   the id of the plugin to fetch
 * @return  the plugin with the specified id
 */
Plugin *DynamicPluginLoader::get_plugin(unsigned int id) const {

  if (id > m_plugin_vect.size())
    return NULL;

  return m_plugin_vect[id];
}
