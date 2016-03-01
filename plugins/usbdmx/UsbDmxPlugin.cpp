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
#include "plugins/usbdmx/PluginImplInterface.h"
#include "plugins/usbdmx/SyncPluginImpl.h"

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
  if (!StringToInt(m_preferences->GetValue(LIBUSB_DEBUG_LEVEL_KEY) ,
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
    return
"USB DMX Plugin\n"
"----------------------------\n"
"\n"
"This plugin supports various USB DMX devices including the \n"
"Anyma uDMX, DMXControl Projects e.V. Nodle U1, Eurolite, Fadecandy, "
"Sunlite USBDMX2 and Velleman K8062.\n"
"\n"
"--- Config file : ola-usbdmx.conf ---\n"
"\n"
"libusb_debug_level = {0,1,2,3,4}\n"
"The debug level for libusb, see http://libusb.sourceforge.net/api-1.0/ .\n"
"0 = No logging, 4 = Verbose debug.\n"
"\n"
"nodle-<serial>-mode = {0,1,2,3,4,5,6,7}\n"
"The mode for the Nodle U1 interface with serial number <serial> "
"to operate in. Default = 6\n"
"0 - Standby\n"
"1 - DMX In -> DMX Out\n"
"2 - PC Out -> DMX Out\n"
"3 - DMX In + PC Out -> DMX Out\n"
"4 - DMX In -> PC In\n"
"5 - DMX In -> DMX Out & DMX In -> PC In\n"
"6 - PC Out -> DMX Out & DMX In -> PC In\n"
"7 - DMX In + PC Out -> DMX Out & DMX In -> PC In\n"
"\n";
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
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
