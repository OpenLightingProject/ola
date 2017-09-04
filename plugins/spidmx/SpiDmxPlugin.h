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
 * SpiDmxPlugin.h
 * This looks for possible SPI devices to instanciate and is managed by OLAD.
 * Copyright (C) 2017 Florian Edelmann
 */

#ifndef PLUGINS_SPIDMX_SPIDMXPLUGIN_H_
#define PLUGINS_SPIDMX_SPIDMXPLUGIN_H_

#include <set>
#include <string>
#include <vector>

#include "olad/Plugin.h"
#include "ola/plugin_id.h"

#include "plugins/spidmx/SpiDmxDevice.h"

namespace ola {
namespace plugin {
namespace spidmx {

class SpiDmxPlugin : public Plugin {
 public:
  explicit SpiDmxPlugin(PluginAdaptor *plugin_adaptor)
      : Plugin(plugin_adaptor),
        m_plugin_adaptor(plugin_adaptor) {
  }

  ola_plugin_id Id() const { return OLA_PLUGIN_SPIDMX; }
  std::string Name() const { return PLUGIN_NAME; }
  std::string PluginPrefix() const { return PLUGIN_PREFIX; }

  // This plugin is disabled unless explicitly enabled by a user.
  bool DefaultMode() const { return false; }

  std::string Description() const;

 private:
  typedef std::vector<SpiDmxDevice*> SpiDmxDeviceVector;
  SpiDmxDeviceVector m_devices;

  PluginAdaptor *m_plugin_adaptor;

  void AddDevice(SpiDmxDevice *device);
  bool StartHook();
  bool StopHook();
  bool SetDefaultPreferences();

  static const char PLUGIN_NAME[];
  static const char PLUGIN_PREFIX[];
  static const char PREF_DEVICE_PREFIX_DEFAULT[];
  static const char PREF_DEVICE_PREFIX_KEY[];

  DISALLOW_COPY_AND_ASSIGN(SpiDmxPlugin);
};

}  // namespace spidmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPIDMX_SPIDMXPLUGIN_H_
