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
 * FtdiDmxPlugin.h
 * The FTDI usb chipset DMX plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 */

#ifndef PLUGINS_FTDIDMX_FTDIDMXPLUGIN_H_
#define PLUGINS_FTDIDMX_FTDIDMXPLUGIN_H_

#include <set>
#include <string>
#include <vector>

#include "olad/Plugin.h"
#include "ola/plugin_id.h"

#include "plugins/ftdidmx/FtdiDmxDevice.h"

namespace ola {
namespace plugin {
namespace ftdidmx {


class FtdiDmxPlugin : public Plugin {
 public:
  explicit FtdiDmxPlugin(ola::PluginAdaptor *plugin_adaptor)
      : Plugin(plugin_adaptor) {
  }

  ola_plugin_id Id() const { return OLA_PLUGIN_FTDIDMX; }
  std::string Name() const { return PLUGIN_NAME; }
  std::string PluginPrefix() const { return PLUGIN_PREFIX; }
  // This plugin is disabled unless explicitly enabled by a user.
  bool DefaultMode() const { return false; }

  void ConflictsWith(std::set<ola_plugin_id> *conflict_set) const {
    conflict_set->insert(ola::OLA_PLUGIN_USBPRO);
    conflict_set->insert(ola::OLA_PLUGIN_OPENDMX);
  }

  std::string Description() const;

 private:
  typedef std::vector<FtdiDmxDevice*> FtdiDeviceVector;
  FtdiDeviceVector m_devices;

  void AddDevice(FtdiDmxDevice *device);
  bool StartHook();
  bool StopHook();
  bool SetDefaultPreferences();

  static const uint8_t DEFAULT_FREQUENCY = 30;

  static const char K_FREQUENCY[];
  static const char PLUGIN_NAME[];
  static const char PLUGIN_PREFIX[];
};
}  // namespace ftdidmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_FTDIDMX_FTDIDMXPLUGIN_H_
