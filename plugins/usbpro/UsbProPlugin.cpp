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

#include <stdlib.h>
#include <stdio.h>
#include <vector>

#include <ola/Closure.h>
#include <ola/Logging.h>
#include <olad/PluginAdaptor.h>
#include <olad/Preferences.h>

#include "UsbProPlugin.h"
#include "UsbProDevice.h"


/*
 * Entry point to this plugin
 */
extern "C" ola::AbstractPlugin* create(
    const ola::PluginAdaptor *plugin_adaptor) {
  return new ola::usbpro::UsbProPlugin(plugin_adaptor);
}

/*
 * Called when the plugin is unloaded
 */
extern "C" void destroy(ola::AbstractPlugin* plugin) {
  delete plugin;
}

namespace ola {
namespace usbpro {

const string UsbProPlugin::USBPRO_DEVICE_PATH = "/dev/ttyUSB0";
const string UsbProPlugin::USBPRO_DEVICE_NAME = "Enttec Usb Pro Device";
const string UsbProPlugin::PLUGIN_NAME = "UsbPro Plugin";
const string UsbProPlugin::PLUGIN_PREFIX = "usbpro";
const string UsbProPlugin::DEVICE_PATH_KEY = "device";

/*
 * Start the plugin
 */
bool UsbProPlugin::StartHook() {
  vector<string> device_names = m_preferences->GetMultipleValue(DEVICE_PATH_KEY);
  vector<string>::iterator it;

  for (it = device_names.begin(); it != device_names.end(); ++it) {
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
                         dev->GetSocket())
    );
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
"device = /dev/ttyUSB0\n"
"The device to use. Multiple devices are allowed\n";
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

  if (m_preferences->GetValue(DEVICE_PATH_KEY).empty()) {
    m_preferences->SetValue(DEVICE_PATH_KEY, USBPRO_DEVICE_PATH);
    m_preferences->Save();
  }

  // check if this saved correctly
  // we don't want to use it if null
  if (m_preferences->GetValue(DEVICE_PATH_KEY).empty()) {
    return false;
  }
  return true;
}

void UsbProPlugin::DeleteDevice(UsbProDevice *device) {
  m_plugin_adaptor->UnregisterDevice(device);
  device->Stop();
  delete device;
}

} // usbpro
} // ola
