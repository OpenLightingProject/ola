/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * FtdiDmxPlugin.cpp
 * The FTDI usb chipset DMX plugin for ola
 * Copyright (C) 2011 Rui Barreiros
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


const char FtdiDmxPlugin::DEFAULT_FREQUENCY[] = "30";
const char FtdiDmxPlugin::K_FREQUENCY[] = "frequency";
const char FtdiDmxPlugin::PLUGIN_NAME[] = "FTDI USB DMX";
const char FtdiDmxPlugin::PLUGIN_PREFIX[] = "ftdidmx";

/**
 * Attempt to start a device and, if successfull, register it
 * Ownership of the FtdiDmxDevice is transfered to us here.
 */
void FtdiDmxPlugin::AddDevice(FtdiDmxDevice *device) {
  // Check if device is working before adding
  if (device->GetDevice()->SetupOutput() == false) {
    OLA_WARN << "Unable to setup device for output, device ignored "
             << device->Description();
    delete device;
    return;
  }

  if (device->Start()) {
      m_devices.push_back(device);
      m_plugin_adaptor->RegisterDevice(device);
  } else {
    OLA_WARN << "Failed to start FTDI device " << device->Description();
    delete device;
  }
}


/**
 * Fetch a list of all FTDI widgets and create a new device for each of them.
 */
bool FtdiDmxPlugin::StartHook() {
  typedef vector<FtdiWidgetInfo> FtdiWidgetInfoVector;
  FtdiWidgetInfoVector widgets;
  FtdiWidget::Widgets(&widgets);

  FtdiWidgetInfoVector::const_iterator iter;
  for (iter = widgets.begin(); iter != widgets.end(); ++iter) {
    AddDevice(new FtdiDmxDevice(this, *iter, GetFrequency()));
  }
  return true;
}


/**
 * Stop all the devices.
 */
bool FtdiDmxPlugin::StopHook() {
  FtdiDeviceVector::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    m_plugin_adaptor->UnregisterDevice(*iter);
    (*iter)->Stop();
  }
  m_devices.clear();
  return true;
}


/**
 * Return a description for this plugin.
 */
string FtdiDmxPlugin::Description() const {
  return
    "FTDI USB Chipset DMX Plugin\n"
    "---------------------------\n"
    "This plugin is compatible with Enttec OpenDmx and other\n"
    "FTDI chipset based USB to DMX converters where the host\n"
    "needs to create the DMX stream itself and not the interface\n"
    "(the interface has no microprocessor to do so)\n\n"
    "--- Config file : ola-ftdidmx.conf ---\n\n"
    "frequency = 30\n"
    "The DMX stream frequency (30hz to 44hz max are the usual)\n";
}


/**
 * Set the default preferences
 */
bool FtdiDmxPlugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  if (m_preferences->SetDefaultValue(FtdiDmxPlugin::K_FREQUENCY,
                                     IntValidator(1, 44),
                                     DEFAULT_FREQUENCY))
    m_preferences->Save();

  if (m_preferences->GetValue(FtdiDmxPlugin::K_FREQUENCY).empty())
    return false;

  return true;
}


/**
 * Return the frequency as specified in the config file.
 */
int unsigned FtdiDmxPlugin::GetFrequency() {
  unsigned int frequency;

  if (!StringToInt(m_preferences->GetValue(K_FREQUENCY), &frequency))
    StringToInt(DEFAULT_FREQUENCY, &frequency);
  return frequency;
}
}  // ftdidmx
}  // plugin
}  // ola
