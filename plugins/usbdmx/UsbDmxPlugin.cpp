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
 * UsbDmxPlugin.cpp
 * A plugin that uses libusb to communicate with USB devices.
 * Copyright (C) 2006 Simon Newton
 */

#include "plugins/usbdmx/UsbDmxPlugin.h"

#include <string>

#include "ola/Logging.h"
#include "ola/base/Flags.h"
#include "olad/Preferences.h"
#include "plugins/usbdmx/AsyncPluginImpl.h"
#include "plugins/usbdmx/EuroliteProFactory.h"
#include "plugins/usbdmx/PluginImplInterface.h"
#include "plugins/usbdmx/SyncPluginImpl.h"
#include "plugins/usbdmx/UsbDmxPluginDescription.h"

DECLARE_bool(use_async_libusb);

namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;

const char UsbDmxPlugin::PLUGIN_NAME[] = "USB";
const char UsbDmxPlugin::PLUGIN_PREFIX[] = "usbdmx";
const char UsbDmxPlugin::LIBUSB_DEBUG_LEVEL_KEY[] = "libusb_debug_level";
int UsbDmxPlugin::LIBUSB_DEFAULT_DEBUG_LEVEL = 0;
int UsbDmxPlugin::LIBUSB_MAX_DEBUG_LEVEL = 4;


UsbDmxPlugin::UsbDmxPlugin(PluginAdaptor *plugin_adaptor)
    : Plugin(plugin_adaptor) {
}

UsbDmxPlugin::~UsbDmxPlugin() {}

bool UsbDmxPlugin::StartHook() {
  if (m_impl.get()) {
    return true;
  }

  unsigned int debug_level;
  if (!StringToInt(m_preferences->GetValue(LIBUSB_DEBUG_LEVEL_KEY),
                   &debug_level)) {
    debug_level = LIBUSB_DEFAULT_DEBUG_LEVEL;
  }

  std::auto_ptr<PluginImplInterface> impl;
  if (FLAGS_use_async_libusb) {
    impl.reset(
        new AsyncPluginImpl(m_plugin_adaptor, this, debug_level,
                            m_preferences));
  } else {
    impl.reset(
        new SyncPluginImpl(m_plugin_adaptor, this, debug_level, m_preferences));
  }

  if (impl->Start()) {
    m_impl.reset(impl.release());
    return true;
  } else {
    return false;
  }
}

bool UsbDmxPlugin::StopHook() {
  if (m_impl.get()) {
    m_impl->Stop();
  }
  return true;
}

string UsbDmxPlugin::Description() const {
    return plugin_description;
}

bool UsbDmxPlugin::SetDefaultPreferences() {
  if (!m_preferences) {
    return false;
  }

  bool save = m_preferences->SetDefaultValue(
      LIBUSB_DEBUG_LEVEL_KEY,
      UIntValidator(LIBUSB_DEFAULT_DEBUG_LEVEL, LIBUSB_MAX_DEBUG_LEVEL),
      LIBUSB_DEFAULT_DEBUG_LEVEL);

  if (save) {
    m_preferences->Save();
  }

  return true;
}

void UsbDmxPlugin::ConflictsWith(
    std::set<ola_plugin_id>* conflicting_plugins) const {
  if (EuroliteProFactory::IsEuroliteMk2Enabled(m_preferences)) {
    conflicting_plugins->insert(OLA_PLUGIN_FTDIDMX);
    conflicting_plugins->insert(OLA_PLUGIN_STAGEPROFI);
    conflicting_plugins->insert(OLA_PLUGIN_USBPRO);
  }
}

}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
