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
 * DummyPlugin.cpp
 * The Dummy plugin for lla, contains a single dummy device
 * Copyright (C) 2005-2007 Simon Newton
 */

#include <string>
#include <stdlib.h>
#include <stdio.h>

#include <llad/PluginAdaptor.h>

#include "DummyPlugin.h"
#include "DummyDevice.h"

/*
 * Entry point to this plugin
 */
extern "C" lla::AbstractPlugin* create(const lla::PluginAdaptor *plugin_adaptor) {
  return new lla::plugin::DummyPlugin(plugin_adaptor);
}

/*
 * Called when the plugin is unloaded
 */
extern "C" void destroy(lla::AbstractPlugin *plugin) {
  delete plugin;
}


namespace lla {
namespace plugin {

using std::string;

const string DummyPlugin::PLUGIN_NAME = "Dummy Plugin";
const string DummyPlugin::PLUGIN_PREFIX = "dummy";
const string DummyPlugin::DEVICE_NAME = "Dummy Device";

/*
 * Start the plugin
 *
 * Lets keep it simple, one device for this plugin
 */
bool DummyPlugin::StartHook() {
  m_device = new DummyDevice(this, DEVICE_NAME);
  m_device->Start();
  m_plugin_adaptor->RegisterDevice(m_device);
  return true;
}


/*
 * Stop the plugin
 * @return true on sucess, false on failure
 */
bool DummyPlugin::StopHook() {
  if (m_device) {
    bool ret = m_device->Stop();
    m_plugin_adaptor->UnregisterDevice(m_device);
    delete m_device;
    return ret;
  }
  return true;
}


string DummyPlugin::Description() const {
  return
"Dummy Plugin\n"
"----------------------------\n"
"\n"
"The plugin creates a single device with one port. "
"When used as an output port it prints the first two bytes of dmx data to "
"stdout.\n";
}

} // plugin
} // lla
