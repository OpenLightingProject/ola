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
 * PathportPlugin.cpp
 * The Pathport plugin for lla
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>

#include <llad/PluginAdaptor.h>
#include <llad/Preferences.h>

#include "PathportPlugin.h"
#include "PathportDevice.h"

/*
 * Entry point to this plugin
 */
extern "C" lla::AbstractPlugin* create(const lla::PluginAdaptor *plugin_adaptor) {
  return new lla::plugin::PathportPlugin(plugin_adaptor);
}

/*
 * Called when the plugin is unloaded
 */
extern "C" void destroy(lla::AbstractPlugin* plugin) {
  delete plugin;
}


namespace lla {
namespace plugin {

const string PathportPlugin::PATHPORT_NODE_NAME = "lla-Pathport";
const string PathportPlugin::PLUGIN_NAME = "Pathport Plugin";
const string PathportPlugin::PLUGIN_PREFIX = "pathport";


/*
 * Start the plugin
 *
 * For now we just have one device.
 */
bool PathportPlugin::StartHook() {
  /* create new lla device */
  m_device = new PathportDevice(this, "Pathport Device", m_preferences);

  if (!m_device)
    return false;

  if (!m_device->Start()) {
    delete m_device;
    return false;
  }

  // register our descriptors
  for (int i = 0; i < PATHPORT_MAX_SD; i++)
    m_plugin_adaptor->RegisterFD(m_device->get_sd(i), PluginAdaptor::READ, m_device);

  m_plugin_adaptor->RegisterDevice(m_device);
  return true;
}


/*
 * Stop the plugin
 *
 * @return 0 on sucess, -1 on failure
 */
bool PathportPlugin::StopHook() {
  for (int i = 0; i < PATHPORT_MAX_SD; i++)
    m_plugin_adaptor->UnregisterFD(m_device->get_sd(i), PluginAdaptor::READ);

  m_plugin_adaptor->UnregisterDevice(m_device);
  m_device->Stop();
  delete m_device;
  return true;
}


/*
 * Return the description for this plugin
 */
string PathportPlugin::Description() const {
  return
"Pathport Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates a single device with 5 input and 5 output ports.\n"
"\n"
"The universe the port is patched to corresponds with the DMX channels used in the pathport protocol. "
"For example universe 0 is xDMX channels 1 - 512, universe 1 is xDMX channels 513 - 1024.\n"
"\n"
"--- Config file : lla-pathport.conf ---\n"
"\n"
"ip = a.b.c.d\n"
"The ip address to bind to. If not specified it will use the first non-loopback ip.\n"
"\n"
"name = lla-Pathport\n"
"The name of the node.\n";

}


/*
 * load the plugin prefs and default to sensible values
 *
 */
bool PathportPlugin::SetDefaultPreferences() {

  if (!m_preferences)
    return false;

  // we don't worry about ip here
  // if it's non existant it will choose one
  if (m_preferences->GetValue("name").empty()) {
    m_preferences->SetValue("name", PATHPORT_NODE_NAME);
    m_preferences->Save();
  }

  // check if this save correctly
  // we don't want to use it if null
  if (m_preferences->GetValue("name").empty()) {
    return false;
  }

  return true;
}

} //plugin
} //lla
