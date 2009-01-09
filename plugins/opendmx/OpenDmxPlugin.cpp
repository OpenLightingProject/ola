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
 * OpenDmxPlugin.cpp
 * The Open DMX plugin for lla
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include <llad/PluginAdaptor.h>
#include <llad/Preferences.h>

#include "OpenDmxPlugin.h"
#include "OpenDmxDevice.h"

namespace lla {
namespace plugin {

using lla::PluginAdaptor;

const string OpenDmxPlugin::OPENDMX_DEVICE_PATH = "/dev/dmx0";
const string OpenDmxPlugin::OPENDMX_DEVICE_NAME = "OpenDmx USB Device";
const string OpenDmxPlugin::PLUGIN_NAME = "OpenDmx Plugin";
const string OpenDmxPlugin::PLUGIN_PREFIX = "opendmx";


/*
 * Entry point to this plugin
 */
extern "C" lla::AbstractPlugin* create(const PluginAdaptor *plugin_adaptor) {
  return new OpenDmxPlugin(plugin_adaptor);
}

/*
 * Called when the plugin is unloaded
 */
extern "C" void destroy(lla::AbstractPlugin* plug) {
  delete plug;
}


/*
 * Start the plugin
 *
 * For now we just have one device.
 * TODO: scan /dev for devices?
 *   Need to get multi-device support working first :)
 */
bool OpenDmxPlugin::StartHook() {
  int fd;

  /* create new lla device */
  // first check if it's there
  fd = open(m_preferences->GetValue("device").c_str(), O_WRONLY);

  if (fd > 0) {
    close(fd);
    m_device = new OpenDmxDevice(this, OPENDMX_DEVICE_NAME, m_preferences->GetValue("device"));

    if (!m_device)
      return false;

    m_device->Start();
    m_plugin_adaptor->RegisterDevice(m_device);

    return true;
  }
  return true;
}


/*
 * Stop the plugin
 *
 * @return true on success, false on failure
 */
bool OpenDmxPlugin::StopHook() {
  if (m_device) {
    bool ret = m_device->Stop();
    m_plugin_adaptor->UnregisterDevice(m_device);
    delete m_device;
    return ret;
  }
  return true;
}

/*
 * Return the description for this plugin
 *
 */
string OpenDmxPlugin::Description() const {
    return
"OpenDmx Plugin\n"
"----------------------------\n"
"\n"
"The plugin creates a single device with one output port using the Enttec\n"
"Open DMX USB widget.\n\n"
"--- Config file : lla-opendmx.conf ---\n"
"\n"
"device = /dev/dmx0\n"
"The path to the open dmx usb device.\n";
}


/*
 * Load the plugin prefs and default to sensible values
 */
int OpenDmxPlugin::SetDefaultPreferences() {
  if (!m_preferences)
    return -1;

  if (m_preferences->GetValue("device").empty()) {
    m_preferences->SetValue("device", OPENDMX_DEVICE_PATH);
    m_preferences->Save();
  }

  // check if this save correctly
  // we don't want to use it if null
  if (m_preferences->GetValue("device").empty())
    return -1;

  return 0;
}

} //plugins
} //lla
