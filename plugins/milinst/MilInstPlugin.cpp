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
 * MilInstPlugin.cpp
 * The Milford Instruments plugin for ola
 * Copyright (C) 2013 Peter Newman
 */

#include <string>
#include <vector>

#include "ola/Logging.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/milinst/MilInstDevice.h"
#include "plugins/milinst/MilInstPlugin.h"

namespace ola {
namespace plugin {
namespace milinst {

using ola::io::ConnectedDescriptor;
using std::string;
using std::vector;

// Blank default path, so we don't start using a serial port without being asked
const char MilInstPlugin::MILINST_DEVICE_PATH[] = "";
const char MilInstPlugin::PLUGIN_NAME[] = "Milford Instruments";
const char MilInstPlugin::PLUGIN_PREFIX[] = "milinst";
const char MilInstPlugin::DEVICE_KEY[] = "device";

/*
 * Start the plugin
 */
bool MilInstPlugin::StartHook() {
  vector<string> device_names;
  vector<string>::iterator it;
  MilInstDevice *device;

  // fetch device listing
  device_names = m_preferences->GetMultipleValue(DEVICE_KEY);

  for (it = device_names.begin(); it != device_names.end(); ++it) {
    if (it->empty()) {
      OLA_DEBUG << "No path configured for device, please set one in "
          "ola-milinst.conf";
      continue;
    }

    device = new MilInstDevice(this, m_preferences, *it);
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
bool MilInstPlugin::StopHook() {
  vector<MilInstDevice*>::iterator iter;
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
string MilInstPlugin::Description() const {
    return
"Milford Instruments Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates devices with one output port. It currently supports the "
"1-463 DMX Protocol Converter and 1-553 512 Channel Serial to DMX "
"Transmitter.\n"
"\n"
"--- Config file : ola-milinst.conf ---\n"
"\n"
"device = /dev/ttyS0\n"
"The device to use as a path for the serial port. Multiple devices are "
"supported.\n"
"--- Per Device Settings ---\n"
"<device>-type = [1-463 | 1-553]\n"
"The type of interface.\n"
"\n"
"--- 1-553 Specific Per Device Settings ---\n"
"<device>-baudrate = [9600 | 19200]\n"
"The baudrate to connect at.\n"
"\n"
"<device>-channels = [128 | 256 | 512]\n"
"The number of channels to send.\n"
"\n";
}


/*
 * Called when the file descriptor is closed.
 */
int MilInstPlugin::SocketClosed(ConnectedDescriptor *socket) {
  vector<MilInstDevice*>::iterator iter;

  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    if ((*iter)->GetSocket() == socket) {
      break;
    }
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
bool MilInstPlugin::SetDefaultPreferences() {
  if (!m_preferences) {
    return false;
  }

  bool save = false;

  save |= m_preferences->SetDefaultValue(DEVICE_KEY, StringValidator(),
                                         MILINST_DEVICE_PATH);

  if (save) {
    m_preferences->Save();
  }

  // Just check key exists, as we've set it to ""
  if (!m_preferences->HasKey(DEVICE_KEY)) {
    return false;
  }
  return true;
}


/*
 * Cleanup a single device
 */
void MilInstPlugin::DeleteDevice(MilInstDevice *device) {
  m_plugin_adaptor->UnregisterDevice(device);
  device->Stop();
  delete device;
}
}  // namespace milinst
}  // namespace plugin
}  // namespace ola
