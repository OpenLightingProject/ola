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
 * OPCPlugin.h
 * The Open Pixel Control Plugin.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_OPENPIXELCONTROL_OPCPLUGIN_H_
#define PLUGINS_OPENPIXELCONTROL_OPCPLUGIN_H_

#include <string>
#include <vector>
#include "ola/plugin_id.h"
#include "olad/Device.h"
#include "olad/Plugin.h"

namespace ola {
namespace plugin {
namespace openpixelcontrol {

class OPCPlugin: public Plugin {
 public:
  explicit OPCPlugin(PluginAdaptor *plugin_adaptor)
      : Plugin(plugin_adaptor) {}
  ~OPCPlugin();

  std::string Name() const { return PLUGIN_NAME; }
  ola_plugin_id Id() const { return OLA_PLUGIN_OPENPIXELCONTROL; }
  std::string Description() const;
  std::string PluginPrefix() const { return PLUGIN_PREFIX; }

 private:
  typedef std::vector<ola::Device*> OPCDevices;
  OPCDevices m_devices;

  bool StartHook();
  bool StopHook();
  bool SetDefaultPreferences();

  template <typename DeviceClass>
  void AddDevices(const std::string &key);

  static const char LISTEN_KEY[];
  static const char PLUGIN_NAME[];
  static const char PLUGIN_PREFIX[];
  static const char TARGET_KEY[];
};
}  // namespace openpixelcontrol
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OPENPIXELCONTROL_OPCPLUGIN_H_
