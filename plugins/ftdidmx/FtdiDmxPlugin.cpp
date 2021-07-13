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
 * FtdiDmxPlugin.cpp
 * The FTDI usb chipset DMX plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 *
 * Additional modifications to enable support for multiple outputs and
 * additional device ids did change the original structure.
 *
 * by E.S. Rosenberg a.k.a. Keeper of the Keys 5774/2014
 */

#include <vector>
#include <string>

#include "ola/StringUtils.h"
#include "olad/Preferences.h"
#include "olad/PluginAdaptor.h"
#include "plugins/ftdidmx/FtdiDmxPlugin.h"
#include "plugins/ftdidmx/FtdiDmxDevice.h"
#include "plugins/ftdidmx/FtdiWidget.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

using std::string;
using std::vector;

const char FtdiDmxPlugin::K_FREQUENCY[] = "frequency";
const char FtdiDmxPlugin::PLUGIN_NAME[] = "FTDI USB DMX";
const char FtdiDmxPlugin::PLUGIN_PREFIX[] = "ftdidmx";

/**
 * @brief Attempt to start a device and, if successful, register it
 *
 * Ownership of the FtdiDmxDevice is transferred to us here.
 */
void FtdiDmxPlugin::AddDevice(FtdiDmxDevice *device) {
  if (device->Start()) {
      m_devices.push_back(device);
      m_plugin_adaptor->RegisterDevice(device);
  } else {
    OLA_WARN << "Failed to start FTDI device " << device->Description();
    delete device;
  }
}


/**
 * @brief Fetch a list of all FTDI widgets and create a new device for each of them.
 */
bool FtdiDmxPlugin::StartHook() {
  typedef vector<FtdiWidgetInfo> FtdiWidgetInfoVector;
  FtdiWidgetInfoVector widgets;
  FtdiWidget::Widgets(&widgets);

  unsigned int frequency = StringToIntOrDefault(
      m_preferences->GetValue(K_FREQUENCY),
      DEFAULT_FREQUENCY);

  FtdiWidgetInfoVector::const_iterator iter;
  for (iter = widgets.begin(); iter != widgets.end(); ++iter) {
    AddDevice(new FtdiDmxDevice(this, *iter, frequency));
  }
  return true;
}


/**
 * @brief Stop all the devices.
 */
bool FtdiDmxPlugin::StopHook() {
  FtdiDeviceVector::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    m_plugin_adaptor->UnregisterDevice(*iter);
    (*iter)->Stop();
    delete (*iter);
  }
  m_devices.clear();
  return true;
}


/**
 * @brief Return a description for this plugin.
 */
string FtdiDmxPlugin::Description() const {
  return
"FTDI USB Chipset DMX Plugin\n"
"----------------------------\n"
"\n"
"This plugin is compatible with Enttec Open DMX USB and other\n"
"FTDI chipset based USB to DMX converters where the host\n"
"needs to create the DMX stream itself and not the interface\n"
"(the interface has no microprocessor to do so).\n"
"\n"
"--- Config file : ola-ftdidmx.conf ---\n"
"\n"
"frequency = 30\n"
"The DMX stream frequency (30 to 44 Hz max are the usual).\n"
"\n";
}


/**
 * @brief Set the default preferences
 */
bool FtdiDmxPlugin::SetDefaultPreferences() {
  if (!m_preferences) {
    return false;
  }

  if (m_preferences->SetDefaultValue(FtdiDmxPlugin::K_FREQUENCY,
                                     UIntValidator(1, 44),
                                     DEFAULT_FREQUENCY)) {
    m_preferences->Save();
  }

  if (m_preferences->GetValue(FtdiDmxPlugin::K_FREQUENCY).empty()) {
    return false;
  }

  return true;
}
}  // namespace ftdidmx
}  // namespace plugin
}  // namespace ola
