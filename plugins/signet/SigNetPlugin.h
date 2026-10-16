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
 * SigNetPlugin.h
 * Interface for the SigNet plugin.
 * Copyright (C) 2026 Peter Newman
 */

#ifndef PLUGINS_SIGNET_SIGNETPLUGIN_H_
#define PLUGINS_SIGNET_SIGNETPLUGIN_H_

#include <string>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"
#include "plugins/signet/SigNetDevice.h"

namespace ola {
namespace plugin {
namespace signet {

class SigNetDevice;

class SigNetPlugin: public ola::Plugin {
 public:
  explicit SigNetPlugin(ola::PluginAdaptor *plugin_adaptor):
    ola::Plugin(plugin_adaptor),
    m_device(NULL) {}

  std::string Name() const { return PLUGIN_NAME; }
  std::string Description() const;
  ola_plugin_id Id() const { return OLA_PLUGIN_SIGNET; }
  std::string PluginPrefix() const { return PLUGIN_PREFIX; }

 private:
  bool StartHook();
  bool StopHook();
  bool SetDefaultPreferences();

  SigNetDevice *m_device;
  static const uint8_t DEFAULT_PORT_COUNT = 5;

  static const char INPUT_PORT_COUNT_KEY[];
  static const char OUTPUT_PORT_COUNT_KEY[];
  static const char PLUGIN_NAME[];
  static const char PLUGIN_PREFIX[];
};
}  // namespace signet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SIGNET_SIGNETPLUGIN_H_
