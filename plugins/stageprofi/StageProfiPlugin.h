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
 * StageProfiPlugin.h
 * Interface for the stageprofi plugin class
 * Copyright (C) 2006 Simon Newton
 */

#ifndef PLUGINS_STAGEPROFI_STAGEPROFIPLUGIN_H_
#define PLUGINS_STAGEPROFI_STAGEPROFIPLUGIN_H_

#include <memory>
#include <map>
#include <string>
#include "olad/Plugin.h"
#include "ola/network/Socket.h"
#include "ola/plugin_id.h"
#include "plugins/stageprofi/StageProfiDetector.h"

namespace ola {
namespace plugin {
namespace stageprofi {

class StageProfiDevice;

class StageProfiPlugin: public Plugin {
 public:
  explicit StageProfiPlugin(PluginAdaptor *plugin_adaptor)
      : Plugin(plugin_adaptor) {}
  ~StageProfiPlugin();

  std::string Name() const { return PLUGIN_NAME; }
  ola_plugin_id Id() const { return OLA_PLUGIN_STAGEPROFI; }
  std::string Description() const;
  std::string PluginPrefix() const { return PLUGIN_PREFIX; }

 private:
  typedef std::map<std::string, StageProfiDevice*> DeviceMap;

  DeviceMap m_devices;
  std::unique_ptr<class StageProfiDetector> m_detector;

  bool StartHook();
  bool StopHook();
  bool SetDefaultPreferences();
  void NewWidget(const std::string &widget_path,
                 ola::io::ConnectedDescriptor *descriptor);

  void DeviceRemoved(std::string widget_path);
  void DeleteDevice(StageProfiDevice *device);

  static const char STAGEPROFI_DEVICE_PATH[];
  static const char STAGEPROFI_DEVICE_NAME[];
  static const char PLUGIN_NAME[];
  static const char PLUGIN_PREFIX[];
  static const char DEVICE_KEY[];
};
}  // namespace stageprofi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_STAGEPROFI_STAGEPROFIPLUGIN_H_
