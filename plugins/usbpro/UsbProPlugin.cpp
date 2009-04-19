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
 * The UsbPro plugin for lla
 * Copyright (C) 2006-2007 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>
#include <vector>

#include <llad/PluginAdaptor.h>
#include <llad/Preferences.h>
#include <lla/Logging.h>

#include "UsbProPlugin.h"
#include "UsbProDevice.h"


/*
 * Entry point to this plugin
 */
extern "C" lla::AbstractPlugin* create(
    const lla::PluginAdaptor *plugin_adaptor) {
  return new lla::plugin::UsbProPlugin(plugin_adaptor);
}

/*
 * Called when the plugin is unloaded
 */
extern "C" void destroy(lla::AbstractPlugin* plugin) {
  delete plugin;
}

namespace lla {
namespace plugin {

const string UsbProPlugin::USBPRO_DEVICE_PATH = "/dev/ttyUSB0";
const string UsbProPlugin::USBPRO_DEVICE_NAME = "Enttec Usb Pro Device";
const string UsbProPlugin::PLUGIN_NAME = "UsbPro Plugin";
const string UsbProPlugin::PLUGIN_PREFIX = "usbpro";

/*
 * Start the plugin
 *
 */
bool UsbProPlugin::StartHook() {
  UsbProDevice *dev;

  vector<string> device_names = m_preferences->GetMultipleValue("device");
  vector<string>::iterator it;

  for (it = device_names.begin(); it != device_names.end(); ++it) {
    dev = new UsbProDevice(m_plugin_adaptor, this, USBPRO_DEVICE_NAME, *it);

    if (!dev)
      continue;

    if (dev->Start()) {
      delete dev;
      continue;
    }

    m_plugin_adaptor->AddSocket(dev->GetSocket(), this);
    m_plugin_adaptor->RegisterDevice(dev);
    m_devices.push_back(dev);
  }
  return true;
}


/*
 * Stop the plugin
 *
 * @return true on sucess, -1 on failure
 */
bool UsbProPlugin::StopHook() {
  vector<UsbProDevice*>::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
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
"--- Config file : lla-usbpro.conf ---\n"
"\n"
"device = /dev/ttyUSB0\n"
"The device to use. Multiple devices are allowed\n";
}


/*
 * Called when the file descriptor is closed.
 */
void UsbProPlugin::SocketClosed(Socket *socket) {
  vector<UsbProDevice*>::iterator iter;

  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    if ((*iter)->GetSocket() == socket) {
      break;
    }
  }

  if (iter == m_devices.end()) {
    LLA_WARN << "couldn't find the socket";
    return;
  }

  DeleteDevice(*iter);
  m_devices.erase(iter);
}


/*
 * Default to sensible values
 */
int UsbProPlugin::SetDefaultPreferences() {
  if (!m_preferences)
    return -1;

  if (m_preferences->GetValue("device").empty()) {
    m_preferences->SetValue("device", USBPRO_DEVICE_PATH);
    m_preferences->Save();
  }

  // check if this saved correctly
  // we don't want to use it if null
  if (m_preferences->GetValue("device").empty()) {
    delete m_preferences;
    return -1;
  }
  return 0;
}

void UsbProPlugin::DeleteDevice(UsbProDevice *device) {
  m_plugin_adaptor->RemoveSocket(device->GetSocket());
  device->Stop();
  m_plugin_adaptor->UnregisterDevice(device);
  delete device;
}

} // plugins
} //lla
