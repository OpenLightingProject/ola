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
 * FtdiDmxPlugin.h
 * The FTDI usb chipset DMX plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 */

#include <vector>
#include <string>

#include "olad/Preferences.h"
#include "olad/PluginAdaptor.h"
#include "plugins/ftdidmx/FtdiDmxPlugin.h"
#include "plugins/ftdidmx/FtdiDmxDevice.h"
#include "plugins/ftdidmx/FtdiWidget.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

using ola::PluginAdaptor;

const char FtdiDmxPlugin::PLUGIN_NAME[] = "FTDI USB Chipset Serial DMX";
const char FtdiDmxPlugin::PLUGIN_PREFIX[] = "ftdidmx";
const char FtdiDmxPlugin::DEFAULT_FREQUENCY[] = "30";
const char FtdiDmxPlugin::K_FREQUENCY[] = "frequency";

/*
 * Entry point to this plugin
 */
extern "C" ola::AbstractPlugin* create(PluginAdaptor *plugin_adaptor) {
  return new FtdiDmxPlugin(plugin_adaptor);
}

FtdiDmxPlugin::FtdiDmxPlugin(PluginAdaptor *plugin_adaptor)
  : Plugin(plugin_adaptor) {
  m_devices.clear();
}

void FtdiDmxPlugin::AddDevice(FtdiDmxDevice *device) {
  if (!device->Start()) {
    delete device;
    return;
  }

  m_devices.push_back(device);
  m_plugin_adaptor->RegisterDevice(device);
}

void FtdiDmxPlugin::DeviceRemoved(FtdiDmxDevice *device) {
  vector<FtdiDmxDevice*>::iterator iter;

  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    if (*iter == device)
      break;
  }

  if (iter == m_devices.end()) {
    OLA_WARN << "Couldn't find the removed device!";
    return;
  }

  DeleteDevice(device);
  m_devices.erase(iter);
}

void FtdiDmxPlugin::DeleteDevice(FtdiDmxDevice *device) {
  m_plugin_adaptor->UnregisterDevice(device);
  device->Stop();
  delete device;
}

bool FtdiDmxPlugin::StartHook() {
  vector<FtdiWidgetInfo> devices = FtdiWidget::Widgets();

  for (vector<FtdiWidgetInfo>::iterator iter = devices.begin();
       iter != devices.end(); ++iter) {
    FtdiWidgetInfo nfo = (*iter);
    AddDevice(new FtdiDmxDevice(this, nfo, m_preferences));
  }

  return true;
}

bool FtdiDmxPlugin::StopHook() {
  vector<FtdiDmxDevice*>::iterator iter;

  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    DeleteDevice((*iter));
  }

  m_devices.clear();
  return true;
}

string FtdiDmxPlugin::Description() const {
  return
    "FTDI USB Chipset DMX Plugin\n"
    "---------------------------\n"
    "This plugin is compatible with Enttec OpenDmx and all other\n"
    "FTDI chipset based USB to DMX converters where the plugin\n"
    "needs to create the DMX stream itself and not the interface\n"
    "(the interface has no microprocessor to do so)\n\n"
    "--- Config file : ola-ftdidmx.conf ---\n\n"
    "frequency = 30\n"
    "The DMX stream frequency (30hz to 44hz max are the usual)\n";
}

bool FtdiDmxPlugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  if (m_preferences->SetDefaultValue(FtdiDmxPlugin::K_FREQUENCY,
                                     IntValidator(30, 44),
                                     DEFAULT_FREQUENCY))
    m_preferences->Save();

  if (m_preferences->GetValue(FtdiDmxPlugin::K_FREQUENCY).empty())
    return false;

  return true;
}
}
}
}
