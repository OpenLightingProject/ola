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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * DynamicPluginLoader.cpp
 * This class is responsible for loading and unloading the plugins
 * Copyright (C) 2005 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
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

#ifdef USE_GPIO
#include "plugins/gpio/GPIOPlugin.h"
#endif

#ifdef USE_KARATE
#include "plugins/karate/KaratePlugin.h"
#endif

#ifdef USE_KINET
#include "plugins/kinet/KiNetPlugin.h"
#endif

#ifdef USE_MILINST
#include "plugins/milinst/MilInstPlugin.h"
#endif

#ifdef USE_OPENDMX
#include "plugins/opendmx/OpenDmxPlugin.h"
#endif

#ifdef USE_OPENPIXELCONTROL
#include "plugins/openpixelcontrol/OPCPlugin.h"
#endif

#ifdef USE_OSC
#include "plugins/osc/OSCPlugin.h"
#endif

#ifdef USE_PATHPORT
#include "plugins/pathport/PathportPlugin.h"
#endif

#ifdef USE_RENARD
#include "plugins/renard/RenardPlugin.h"
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

#ifdef USE_LIBUSB
#include "plugins/usbdmx/UsbDmxPlugin.h"
#endif

#ifdef USE_FTDI
#include "plugins/ftdidmx/FtdiDmxPlugin.h"
#endif

#ifdef USE_UART
#include "plugins/uartdmx/UartDmxPlugin.h"
#endif

#ifdef USE_DMX4LINUX
#include "plugins/dmx4linux/Dmx4LinuxPlugin.h"
#endif

namespace ola {

using std::vector;

DynamicPluginLoader::~DynamicPluginLoader() {
  UnloadPlugins();
}

vector<AbstractPlugin*> DynamicPluginLoader::LoadPlugins() {
  if (m_plugins.empty()) {
    PopulatePlugins();
  }
  return m_plugins;
}

/*
 * Setup the plugin list
 */
void DynamicPluginLoader::PopulatePlugins() {
#ifdef USE_DMX4LINUX
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

#ifdef USE_GPIO
  m_plugins.push_back(new ola::plugin::gpio::GPIOPlugin(m_plugin_adaptor));
#endif

#ifdef USE_KARATE
  m_plugins.push_back(
      new ola::plugin::karate::KaratePlugin(m_plugin_adaptor));
#endif

#ifdef USE_KINET
  m_plugins.push_back(new ola::plugin::kinet::KiNetPlugin(m_plugin_adaptor));
#endif

#ifdef USE_MILINST
  m_plugins.push_back(
      new ola::plugin::milinst::MilInstPlugin(m_plugin_adaptor));
#endif

#ifdef USE_OPENDMX
  m_plugins.push_back(
      new ola::plugin::opendmx::OpenDmxPlugin(m_plugin_adaptor));
#endif

#ifdef USE_OPENPIXELCONTROL
  m_plugins.push_back(
      new ola::plugin::openpixelcontrol::OPCPlugin(m_plugin_adaptor));
#endif

#ifdef USE_OSC
  m_plugins.push_back(
      new ola::plugin::osc::OSCPlugin(m_plugin_adaptor));
#endif

#ifdef USE_RENARD
  m_plugins.push_back(
      new ola::plugin::renard::RenardPlugin(m_plugin_adaptor));
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

#ifdef USE_LIBUSB
  m_plugins.push_back(new ola::plugin::usbdmx::UsbDmxPlugin(m_plugin_adaptor));
#endif

#ifdef USE_PATHPORT
  m_plugins.push_back(
      new ola::plugin::pathport::PathportPlugin(m_plugin_adaptor));
#endif

#ifdef USE_FTDI
  m_plugins.push_back(
      new ola::plugin::ftdidmx::FtdiDmxPlugin(m_plugin_adaptor));
#endif

#ifdef USE_UART
  m_plugins.push_back(
      new ola::plugin::uartdmx::UartDmxPlugin(m_plugin_adaptor));
#endif
}

void DynamicPluginLoader::UnloadPlugins() {
  STLDeleteElements(&m_plugins);
}
}  // namespace ola
