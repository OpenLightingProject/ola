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
 * ArtNetPlugin.h
 * Interface for the Art-Net plugin class
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_ARTNET_ARTNETPLUGIN_H_
#define PLUGINS_ARTNET_ARTNETPLUGIN_H_

#include <string>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"

namespace ola {
namespace plugin {
namespace artnet {

class ArtNetDevice;

class ArtNetPlugin : public Plugin {
 public:
  explicit ArtNetPlugin(PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor),
      m_device(NULL) {}

  ~ArtNetPlugin() {}

  std::string Name() const { return PLUGIN_NAME; }
  ola_plugin_id Id() const { return OLA_PLUGIN_ARTNET; }
  std::string Description() const;
  std::string PluginPrefix() const { return PLUGIN_PREFIX; }

 private:
  /**
   * Start the plugin, for now we just have one device.
   * @todo Allow multiple devices on different IPs?
   * @return true if we started ok, false otherwise
   */
  bool StartHook();
  bool StopHook();
  bool SetDefaultPreferences();

  ArtNetDevice *m_device;  // only have one device

  static const char ARTNET_NET[];
  static const char ARTNET_SUBNET[];
  static const char ARTNET_LONG_NAME[];
  static const char ARTNET_SHORT_NAME[];
  static const char PLUGIN_NAME[];
  static const char PLUGIN_PREFIX[];
};
}  // namespace artnet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_ARTNET_ARTNETPLUGIN_H_
