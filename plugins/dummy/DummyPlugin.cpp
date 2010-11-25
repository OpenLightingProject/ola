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
 * The Dummy plugin for ola, contains a single dummy device
 * Copyright (C) 2005-2007 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "olad/PluginAdaptor.h"
#include "plugins/dummy/DummyDevice.h"
#include "plugins/dummy/DummyPlugin.h"

/*
 * Entry point to this plugin
 */
extern "C" ola::AbstractPlugin* create(
    ola::PluginAdaptor *plugin_adaptor) {
  return new ola::plugin::dummy::DummyPlugin(plugin_adaptor);
}

namespace ola {
namespace plugin {
namespace dummy {

using std::string;

const char DummyPlugin::PLUGIN_NAME[] = "Dummy";
const char DummyPlugin::PLUGIN_PREFIX[] = "dummy";
const char DummyPlugin::DEVICE_NAME[] = "Dummy Device";

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
 * @return true on success, false on failure
 */
bool DummyPlugin::StopHook() {
  if (m_device) {
    m_plugin_adaptor->UnregisterDevice(m_device);
    bool ret = m_device->Stop();
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
"The plugin creates a single device with one port. When used as an output\n"
"port it prints the first two bytes of dmx data to stdout.\n\n"
"It also creates a fake RDM device which can be querried and the DMX start\n"
"address can be changed.\n";
}
}  // dummy
}  // plugin
}  // ola
