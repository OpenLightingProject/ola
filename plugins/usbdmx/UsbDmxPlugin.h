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
 * UsbDmxPlugin.h
 * A plugin that uses libusb to communicate with USB devices.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBDMX_USBDMXPLUGIN_H_
#define PLUGINS_USBDMX_USBDMXPLUGIN_H_

#include <memory>
#include <set>
#include <string>
#include "ola/base/Macro.h"
#include "ola/plugin_id.h"
#include "olad/Plugin.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief A plugin that uses libusb to communicate with USB devices.
 *
 * This plugin supports a number of USB dongles including
 *   - Anyma uDMX.
 *   - AVLdiy D512.
 *   - DMXControl Projects e.V. Nodle U1.
 *   - DMXCreator 512 Basic USB.
 *   - Eurolite DMX USB Pro.
 *   - Eurolite DMX USB Pro MK2.
 *   - Scanlime's Fadecandy.
 *   - Sunlite.
 *   - Velleman K8062.
 */
class UsbDmxPlugin: public ola::Plugin {
 public:
  /**
   * @brief Create a new UsbDmxPlugin.
   * @param plugin_adaptor The PluginAdaptor to use, ownership is not
   *   transferred.
   */
  explicit UsbDmxPlugin(PluginAdaptor *plugin_adaptor);
  ~UsbDmxPlugin();

  std::string Name() const { return PLUGIN_NAME; }
  std::string Description() const;
  ola_plugin_id Id() const { return OLA_PLUGIN_USBDMX; }
  std::string PluginPrefix() const { return PLUGIN_PREFIX; }

  void ConflictsWith(
      std::set<ola_plugin_id>* conflicting_plugins) const;

 private:
  std::auto_ptr<class PluginImplInterface> m_impl;

  bool StartHook();
  bool StopHook();
  bool SetDefaultPreferences();

  static const char PLUGIN_NAME[];
  static const char PLUGIN_PREFIX[];
  static const char LIBUSB_DEBUG_LEVEL_KEY[];
  static int LIBUSB_DEFAULT_DEBUG_LEVEL;
  static int LIBUSB_MAX_DEBUG_LEVEL;

  DISALLOW_COPY_AND_ASSIGN(UsbDmxPlugin);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_USBDMXPLUGIN_H_
