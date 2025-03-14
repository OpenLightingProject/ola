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
 * GPIOPlugin.h
 * The General Purpose digital I/O plugin.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_GPIO_GPIOPLUGIN_H_
#define PLUGINS_GPIO_GPIOPLUGIN_H_

#include <string>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"

namespace ola {
namespace plugin {
namespace gpio {

/**
 * @brief A plugin that drives general purpose digital I/O lines.
 */
class GPIOPlugin: public ola::Plugin {
 public:
  /**
   * @brief Create a new GPIOPlugin.
   * @param plugin_adaptor the PluginAdaptor to use
   */
  explicit GPIOPlugin(class ola::PluginAdaptor *plugin_adaptor)
      : Plugin(plugin_adaptor),
        m_device(NULL) {}

  std::string Name() const { return PLUGIN_NAME; }
  std::string Description() const;
  ola_plugin_id Id() const { return OLA_PLUGIN_GPIO; }
  std::string PluginPrefix() const { return PLUGIN_PREFIX; }

 private:
  class GPIODevice *m_device;

  bool StartHook();
  bool StopHook();
  bool SetDefaultPreferences();

  static const char GPIO_PINS_KEY[];
  static const char GPIO_PINS_INVERTED_KEY[];
  static const char GPIO_SLOT_OFFSET_KEY[];
  static const char GPIO_TURN_OFF_KEY[];
  static const char GPIO_TURN_ON_KEY[];
  static const char PLUGIN_NAME[];
  static const char PLUGIN_PREFIX[];

  DISALLOW_COPY_AND_ASSIGN(GPIOPlugin);
};
}  // namespace gpio
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_GPIO_GPIOPLUGIN_H_
