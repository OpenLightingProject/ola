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
 * StageProfiPlugin.cpp
 * The StageProfi plugin for ola
 * Copyright (C) 2006-2008 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/stageprofi/StageProfiDevice.h"
#include "plugins/stageprofi/StageProfiPlugin.h"

namespace ola {
namespace plugin {
namespace stageprofi {

using std::string;

const char StageProfiPlugin::STAGEPROFI_DEVICE_PATH[] = "/dev/ttyUSB0";
const char StageProfiPlugin::STAGEPROFI_DEVICE_NAME[] = "StageProfi Device";
const char StageProfiPlugin::PLUGIN_NAME[] = "StageProfi";
const char StageProfiPlugin::PLUGIN_PREFIX[] = "stageprofi";
const char StageProfiPlugin::DEVICE_KEY[] = "device";

/*
 * Start the plugin
 *
 * Multiple devices now supported
 */
bool StageProfiPlugin::StartHook() {
  vector<string> device_names;
  vector<string>::iterator it;
  StageProfiDevice *device;

  // fetch device listing
  device_names = m_preferences->GetMultipleValue(DEVICE_KEY);

  for (it = device_names.begin(); it != device_names.end(); ++it) {
    if (it->empty())
      continue;

    device = new StageProfiDevice(this, STAGEPROFI_DEVICE_NAME, *it);

    if (!device->Start()) {
      delete device;
      continue;
    }

    m_plugin_adaptor->AddReadDescriptor(device->GetSocket());
    m_plugin_adaptor->RegisterDevice(device);
    m_devices.insert(m_devices.end(), device);
  }
  return true;
}


/*
 * Stop the plugin
 * @return true on success, false on failure
 */
bool StageProfiPlugin::StopHook() {
  vector<StageProfiDevice*>::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    m_plugin_adaptor->RemoveReadDescriptor((*iter)->GetSocket());
    DeleteDevice(*iter);
  }
  m_devices.clear();
  return true;
}


/*
 * Return the description for this plugin
 */
string StageProfiPlugin::Description() const {
    return
"StageProfi Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates devices with one output port.\n"
"\n"
"--- Config file : ola-stageprofi.conf ---\n"
"\n"
"device = /dev/ttyUSB0\n"
"device = 192.168.1.250\n"
"The device to use either as a path for the USB version or an IP address\n"
"for the LAN version. Multiple devices are supported.\n";
}


/*
 * Called when the file descriptor is closed.
 */
int StageProfiPlugin::SocketClosed(ConnectedDescriptor *socket) {
  vector<StageProfiDevice*>::iterator iter;

  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    if ((*iter)->GetSocket() == socket)
      break;
  }

  if (iter == m_devices.end()) {
    OLA_WARN << "unknown fd";
    return -1;
  }

  DeleteDevice(*iter);
  m_devices.erase(iter);
  return 0;
}


/*
 * load the plugin prefs and default to sensible values
 *
 */
bool StageProfiPlugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  bool save = false;

  save |= m_preferences->SetDefaultValue(DEVICE_KEY, StringValidator(),
                                         STAGEPROFI_DEVICE_PATH);

  if (save)
    m_preferences->Save();

  if (m_preferences->GetValue(DEVICE_KEY).empty())
    return false;
  return true;
}


/*
 * Cleanup a single device
 */
void StageProfiPlugin::DeleteDevice(StageProfiDevice *device) {
  m_plugin_adaptor->UnregisterDevice(device);
  device->Stop();
  delete device;
}
}  // stageprofi
}  // plugin
}  // ola
