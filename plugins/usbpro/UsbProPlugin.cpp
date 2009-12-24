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
 * UsbProPlugin.cpp
 * The UsbPro plugin for ola
 * Copyright (C) 2006-2007 Simon Newton
 */

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "ola/Closure.h"
#include "ola/Logging.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"

#include "plugins/usbpro/UsbProPlugin.h"
#include "plugins/usbpro/UsbProDevice.h"


/*
 * Entry point to this plugin
 */
extern "C" ola::AbstractPlugin* create(
    const ola::PluginAdaptor *plugin_adaptor) {
  return new ola::plugin::usbpro::UsbProPlugin(plugin_adaptor);
}


namespace ola {
namespace plugin {
namespace usbpro {

const char UsbProPlugin::USBPRO_DEVICE_NAME[] = "Enttec Usb Pro Device";
const char UsbProPlugin::PLUGIN_NAME[] = "UsbPro Plugin";
const char UsbProPlugin::PLUGIN_PREFIX[] = "usbpro";
const char UsbProPlugin::DEVICE_DIR_KEY[] = "device_dir";
const char UsbProPlugin::DEVICE_PREFIX_KEY[] = "device_prefix";
const char UsbProPlugin::DEFAULT_DEVICE_DIR[] = "/dev";
const char UsbProPlugin::LINUX_DEVICE_PREFIX[] = "ttyUSB";
const char UsbProPlugin::MAC_DEVICE_PREFIX[] = "cu.usbserial-";

/*
 * Start the plugin
 */
bool UsbProPlugin::StartHook() {
  vector<string>::iterator it;

  vector<string> device_paths = FindCandiateDevices();

  for (it = device_paths.begin(); it != device_paths.end(); ++it) {
    UsbProDevice *dev = new UsbProDevice(m_plugin_adaptor,
                                         this,
                                         USBPRO_DEVICE_NAME,
                                         *it);
    if (!dev->Start()) {
      delete dev;
      continue;
    }

    // We don't register the device here, that's done asyncronously when the
    // startup sequence completes.
    dev->GetSocket()->SetOnClose(
        NewSingleClosure(this,
                         &UsbProPlugin::SocketClosed,
                         dev->GetSocket()));
    m_devices.push_back(dev);
  }
  return true;
}


/*
 * Stop the plugin
 * @return true on sucess, false on failure
 */
bool UsbProPlugin::StopHook() {
  vector<UsbProDevice*>::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    m_plugin_adaptor->RemoveSocket((*iter)->GetSocket());
    DeleteDevice(*iter);
  }
  m_devices.clear();
  return true;
}


/*
 * Return the description for this plugin
 */
string UsbProPlugin::Description() const {
    return
"Enttec Usb Pro Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates devices with one input and one output port.\n"
"\n"
"--- Config file : ola-usbpro.conf ---\n"
"\n"
"device_dir = /dev\n"
"The directory to look for devices in]\n"
"\n"
"device_prefix = ttyUSB\n"
"The prefix of filenames to consider as devices, multiple keys are allowed\n";
}


/*
 * Called when the file descriptor is closed.
 */
int UsbProPlugin::SocketClosed(ConnectedSocket *socket) {
  vector<UsbProDevice*>::iterator iter;

  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    if ((*iter)->GetSocket() == socket) {
      break;
    }
  }

  if (iter == m_devices.end()) {
    OLA_WARN << "Couldn't find the device corresponding to this socket";
    return -1;
  }

  DeleteDevice(*iter);
  m_devices.erase(iter);
  return 0;
}


/*
 * Default to sensible values
 */
bool UsbProPlugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  bool save = false;

  vector<string> device_prefixes =
    m_preferences->GetMultipleValue(DEVICE_PREFIX_KEY);
  if (!device_prefixes.size()) {
    m_preferences->SetMultipleValue(DEVICE_PREFIX_KEY, LINUX_DEVICE_PREFIX);
    m_preferences->SetMultipleValue(DEVICE_PREFIX_KEY, MAC_DEVICE_PREFIX);
    save = true;
  }

  if (m_preferences->GetValue(DEVICE_DIR_KEY).empty()) {
    m_preferences->SetValue(DEVICE_DIR_KEY, DEFAULT_DEVICE_DIR);
    save = true;
  }

  if (save)
    m_preferences->Save();

  device_prefixes = m_preferences->GetMultipleValue(DEVICE_PREFIX_KEY);
  if (!device_prefixes.size())
    return false;
  return true;
}


void UsbProPlugin::DeleteDevice(UsbProDevice *device) {
  m_plugin_adaptor->UnregisterDevice(device);
  device->Stop();
  delete device;
}


/*
 * Look for candidate devices in /dev
 * @returns a list of paths that may be usb pro devices
 */
vector<string> UsbProPlugin::FindCandiateDevices() {
  vector<string> device_paths;

  vector<string> device_prefixes =
    m_preferences->GetMultipleValue(DEVICE_PREFIX_KEY);
  string device_dir = m_preferences->GetValue(DEVICE_DIR_KEY);

  if (device_prefixes.size()) {
    DIR *dp;
    struct dirent dir_ent;
    struct dirent *dir_ent_p;
    if ((dp  = opendir(device_dir.data())) == NULL) {
        OLA_WARN << "Could not open " << device_dir << ":" << strerror(errno);
        return device_paths;
    }

    readdir_r(dp, &dir_ent, &dir_ent_p);
    while (dir_ent_p != NULL) {
      vector<string>::const_iterator iter;
      for (iter = device_prefixes.begin(); iter != device_prefixes.end();
           ++iter) {
        if (!strncmp(dir_ent_p->d_name, iter->data(), iter->size())) {
          stringstream str;
          str << device_dir << "/" << dir_ent_p->d_name;
          device_paths.push_back(str.str());
          OLA_INFO << "Found potential USB Pro device " << str.str();
        }
      }
      readdir_r(dp, &dir_ent, &dir_ent_p);
    }
    closedir(dp);
  }
  return device_paths;
}
}  // usbpro
}  // plugin
}  // ola
