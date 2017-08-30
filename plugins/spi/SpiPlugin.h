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
 * SpiPlugin.h
 * An SPI plugin.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_SPI_SPIPLUGIN_H_
#define PLUGINS_SPI_SPIPLUGIN_H_

#include <string>
#include <vector>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"

namespace ola {
namespace plugin {
namespace spi {

class SpiPlugin: public ola::Plugin {
 public:
  explicit SpiPlugin(class ola::PluginAdaptor *plugin_adaptor)
      : Plugin(plugin_adaptor) {}

  std::string Name() const { return PLUGIN_NAME; }
  std::string Description() const;
  ola_plugin_id Id() const { return OLA_PLUGIN_SPI; }
  std::string PluginPrefix() const { return PLUGIN_PREFIX; }

 private:
  std::vector<class SpiDevice*> m_devices;

  bool StartHook();
  bool StopHook();
  bool SetDefaultPreferences();

  static const char DEFAULT_BASE_UID[];
  static const char DEFAULT_SPI_DEVICE_PREFIX[];
  static const char PLUGIN_NAME[];
  static const char PLUGIN_PREFIX[];
  static const char SPI_BASE_UID_KEY[];
  static const char SPI_DEVICE_PREFIX_KEY[];
};
}  // namespace spi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPI_SPIPLUGIN_H_
