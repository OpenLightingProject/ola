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
 * Copyright (C) 2005-2009 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <vector>
#include "olad/Plugin.h"
#include "olad/DynamicPluginLoader.h"

#include "plugins/dummy/DummyPlugin.h"
#include "plugins/e131/E131Plugin.h"
#include "plugins/espnet/EspNetPlugin.h"
#include "plugins/opendmx/OpenDmxPlugin.h"
#include "plugins/pathport/PathportPlugin.h"
#include "plugins/sandnet/SandNetPlugin.h"
#include "plugins/shownet/ShowNetPlugin.h"
#include "plugins/stageprofi/StageProfiPlugin.h"
#include "plugins/usbpro/UsbProPlugin.h"

#ifdef HAVE_ARTNET
#include "plugins/artnet/ArtNetPlugin.h"
#endif


#ifdef HAVE_DMX4LINUX
#include "plugins/dmx4linux/Dmx4LinuxPlugin.h"
#endif


namespace ola {

using std::vector;

/*
 * Load the plugins that we were linked against
 * @returns a vector of plugins
 */
vector<AbstractPlugin*> DynamicPluginLoader::LoadPlugins() {
  vector<AbstractPlugin*> plugins;

#ifdef HAVE_ARTNET
  plugins.push_back(new ola::plugin::artnet::ArtNetPlugin(m_plugin_adaptor));
#endif

#ifdef HAVE_DMX4LINUX
  plugins.push_back(
      new ola::plugin::dmx4linux::Dmx4LinuxPlugin(m_plugin_adaptor));
#endif

  plugins.push_back(new ola::plugin::dummy::DummyPlugin(m_plugin_adaptor));
  plugins.push_back(new ola::plugin::e131::E131Plugin(m_plugin_adaptor));
  plugins.push_back(new ola::plugin::espnet::EspNetPlugin(m_plugin_adaptor));
  plugins.push_back(
      new ola::plugin::opendmx::OpenDmxPlugin(m_plugin_adaptor));
  plugins.push_back(
      new ola::plugin::sandnet::SandNetPlugin(m_plugin_adaptor));
  plugins.push_back(
      new ola::plugin::shownet::ShowNetPlugin(m_plugin_adaptor));
  plugins.push_back(
      new ola::plugin::stageprofi::StageProfiPlugin(m_plugin_adaptor));
  plugins.push_back(new ola::plugin::usbpro::UsbProPlugin(m_plugin_adaptor));
  plugins.push_back(
      new ola::plugin::pathport::PathportPlugin(m_plugin_adaptor));
  return plugins;
}
}  // ola
