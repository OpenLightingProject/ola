/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * DynamicPluginLoader.cpp
 * This class is responsible for loading and unloading the plugins
 * Copyright (C) 2005-2009 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <vector>
#include "ola/stl/STLUtils.h"
#include "olad/DynamicPluginLoader.h"
#include "olad/Plugin.h"

#ifdef USE_ARTNET
#include "plugins/artnet/ArtNetPlugin.h"
#endif

#ifdef USE_DUMMY
#include "plugins/dummy/DummyPlugin.h"
#endif

#ifdef USE_E131
#include "plugins/e131/E131Plugin.h"
#endif

#ifdef USE_ESPNET
#include "plugins/espnet/EspNetPlugin.h"
#endif

#ifdef USE_KINET
#include "plugins/kinet/KiNetPlugin.h"
#endif

#ifdef USE_OPENDMX
#include "plugins/opendmx/OpenDmxPlugin.h"
#endif

#ifdef USE_PATHPORT
#include "plugins/pathport/PathportPlugin.h"
#endif

#ifdef USE_SANDNET
#include "plugins/sandnet/SandNetPlugin.h"
#endif

#ifdef USE_SHOWNET
#include "plugins/shownet/ShowNetPlugin.h"
#endif

#ifdef USE_SPI
#include "plugins/spi/SPIPlugin.h"
#endif

#ifdef USE_STAGEPROFI
#include "plugins/stageprofi/StageProfiPlugin.h"
#endif

#ifdef USE_USBPRO
#include "plugins/usbpro/UsbSerialPlugin.h"
#endif

#ifdef HAVE_LIBUSB
#include "plugins/usbdmx/UsbDmxPlugin.h"
#endif

#ifdef HAVE_LIBFTDI
#include "plugins/ftdidmx/FtdiDmxPlugin.h"
#endif

#ifdef HAVE_DMX4LINUX
#include "plugins/dmx4linux/Dmx4LinuxPlugin.h"
#endif

#ifdef HAVE_LIBLO
#include "plugins/osc/OSCPlugin.h"
#endif

namespace ola {

using std::vector;


DynamicPluginLoader::~DynamicPluginLoader() {
  STLDeleteElements(&m_plugins);
}


/*
 * Return the plugins that we were linked against
 * @returns a vector of plugins
 */
vector<AbstractPlugin*> DynamicPluginLoader::LoadPlugins() {
  if (m_plugins.empty())
    PopulatePlugins();
  return m_plugins;
}


/**
 * Setup the plugin list
 */
void DynamicPluginLoader::PopulatePlugins() {
#ifdef HAVE_DMX4LINUX
  m_plugins.push_back(
      new ola::plugin::dmx4linux::Dmx4LinuxPlugin(m_plugin_adaptor));
#endif
#ifdef USE_ARTNET
  m_plugins.push_back(new ola::plugin::artnet::ArtNetPlugin(m_plugin_adaptor));
#endif

#ifdef USE_DUMMY
  m_plugins.push_back(new ola::plugin::dummy::DummyPlugin(m_plugin_adaptor));
#endif

#ifdef USE_E131
  m_plugins.push_back(new ola::plugin::e131::E131Plugin(m_plugin_adaptor));
#endif

#ifdef USE_ESPNET
  m_plugins.push_back(new ola::plugin::espnet::EspNetPlugin(m_plugin_adaptor));
#endif

#ifdef USE_KINET
  m_plugins.push_back(new ola::plugin::kinet::KiNetPlugin(m_plugin_adaptor));
#endif

#ifdef USE_OPENDMX
  m_plugins.push_back(
      new ola::plugin::opendmx::OpenDmxPlugin(m_plugin_adaptor));
#endif

#ifdef HAVE_LIBLO
  m_plugins.push_back(
      new ola::plugin::osc::OSCPlugin(m_plugin_adaptor));
#endif

#ifdef USE_SANDNET
  m_plugins.push_back(
      new ola::plugin::sandnet::SandNetPlugin(m_plugin_adaptor));
#endif

#ifdef USE_SHOWNET
  m_plugins.push_back(
      new ola::plugin::shownet::ShowNetPlugin(m_plugin_adaptor));
#endif

#ifdef USE_SPI
  m_plugins.push_back(
      new ola::plugin::spi::SPIPlugin(m_plugin_adaptor));
#endif

#ifdef USE_STAGEPROFI
  m_plugins.push_back(
      new ola::plugin::stageprofi::StageProfiPlugin(m_plugin_adaptor));
#endif

#ifdef USE_USBPRO
  m_plugins.push_back(
      new ola::plugin::usbpro::UsbSerialPlugin(m_plugin_adaptor));
#endif

#ifdef HAVE_LIBUSB
  m_plugins.push_back(new ola::plugin::usbdmx::UsbDmxPlugin(m_plugin_adaptor));
#endif

#ifdef USE_PATHPORT
  m_plugins.push_back(
      new ola::plugin::pathport::PathportPlugin(m_plugin_adaptor));
#endif

#ifdef HAVE_LIBFTDI
  m_plugins.push_back(
      new ola::plugin::ftdidmx::FtdiDmxPlugin(m_plugin_adaptor));
#endif
}
}  // namespace ola
