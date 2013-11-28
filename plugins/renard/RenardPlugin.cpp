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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * RenardPlugin.cpp
 * The Renard plugin for ola
 * Copyright (C) 2013 Hakan Lindestaf
 */

#include <string>
#include <vector>

#include "ola/Logging.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/renard/RenardDevice.h"
#include "plugins/renard/RenardPlugin.h"

namespace ola {
namespace plugin {
namespace renard {

using ola::io::ConnectedDescriptor;
using std::string;

// Blank default path, so we don't start using a serial port without being asked
const char RenardPlugin::RENARD_DEVICE_PATH[] = "";
const char RenardPlugin::PLUGIN_NAME[] = "Renard";
const char RenardPlugin::PLUGIN_PREFIX[] = "renard";
const char RenardPlugin::DEVICE_KEY[] = "device";

/*
 * Start the plugin
 */
bool RenardPlugin::StartHook() {
  vector<string> device_names;
  vector<string>::iterator it;
  RenardDevice *device;

  // fetch device listing
  device_names = m_preferences->GetMultipleValue(DEVICE_KEY);

  for (it = device_names.begin(); it != device_names.end(); ++it) {
    if (it->empty()) {
      OLA_DEBUG << "No path configured for device, please set one in "
          "ola-renard.conf";
      continue;
    }

    // TODO(Peter): When support is added for multiple device types, ensure the
    // correct name is passed in here
    device = new RenardDevice(this, m_preferences, *it);
    OLA_DEBUG << "Adding device " << *it;

    if (!device->Start()) {
      delete device;
      continue;
    }

    OLA_DEBUG << "Started device " << *it;

    m_plugin_adaptor->AddReadDescriptor(device->GetSocket());
    m_plugin_adaptor->RegisterDevice(device);
    m_devices.push_back(device);
  }
  return true;
}


/*
 * Stop the plugin
 * @return true on success, false on failure
 */
bool RenardPlugin::StopHook() {
  vector<RenardDevice*>::iterator iter;
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
string RenardPlugin::Description() const {
    return
"Renard Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates devices with one output port. It supports multiple "
"Renard SS boards daisy-chained.\n"
"\n"
"--- Config file : ola-renard.conf ---\n"
"\n"
"device = /dev/ttyUSB0\n"
"The device to use as a path for the serial port. Multiple devices are "
"supported.\n"
"\n"
"--- Per Device Settings (using above device name without /dev/) ---\n"
"<device>-baudrate = <int>\n"
"The speed of the serial port, options are 9600, 19200, 38400, 57600, "
"115200. Default is 57600.\n"
"\n"
"<device>-channels = <int>\n"
"The number of channels connected to this device. Default 64.\n"
"\n"
"<device>-dmx-offset = <int>\n"
"Which starting point in the DMX universe this device is mapped to. "
"The default is 0, which means the first channel on Renard address "
"128 (0x80) will be mapped to DMX channel 1.\n"
"\n";
}


/*
 * Called when the file descriptor is closed.
 */
int RenardPlugin::SocketClosed(ConnectedDescriptor *socket) {
  vector<RenardDevice*>::iterator iter;

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
bool RenardPlugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  bool save = false;

  save |= m_preferences->SetDefaultValue(DEVICE_KEY, StringValidator(),
                                         RENARD_DEVICE_PATH);

  if (save)
    m_preferences->Save();

  // Just check key exists, as we've set it to ""
  if (!m_preferences->HasKey(DEVICE_KEY))
    return false;
  return true;
}


/*
 * Cleanup a single device
 */
void RenardPlugin::DeleteDevice(RenardDevice *device) {
  m_plugin_adaptor->UnregisterDevice(device);
  device->Stop();
  delete device;
}
}  // namespace renard
}  // namespace plugin
}  // namespace ola
