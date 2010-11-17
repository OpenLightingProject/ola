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
 * ShowNetPlugin.cpp
 * The ShowNet plugin for ola
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string>
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/shownet/ShowNetDevice.h"
#include "plugins/shownet/ShowNetPlugin.h"


/*
 * Entry point to this plugin
 */
extern "C" ola::AbstractPlugin* create(const ola::PluginAdaptor *adaptor) {
  return new ola::plugin::shownet::ShowNetPlugin(adaptor);
}


namespace ola {
namespace plugin {
namespace shownet {


const char ShowNetPlugin::SHOWNET_NODE_NAME[] = "ola-ShowNet";
const char ShowNetPlugin::PLUGIN_NAME[] = "ShowNet";
const char ShowNetPlugin::PLUGIN_PREFIX[] = "shownet";
const char ShowNetPlugin::SHOWNET_NAME_KEY[] = "name";

/*
 * Start the plugin
 */
bool ShowNetPlugin::StartHook() {
  m_device = new ShowNetDevice(this, m_preferences, m_plugin_adaptor);

  if (!m_device->Start()) {
    delete m_device;
    return false;
  }

  m_plugin_adaptor->RegisterDevice(m_device);
  return true;
}


/*
 * Stop the plugin
 * @return true on success, false on failure
 */
bool ShowNetPlugin::StopHook() {
  if (m_device) {
    m_plugin_adaptor->UnregisterDevice(m_device);
    bool ret = m_device->Stop();
    delete m_device;
    return ret;
  }
  return true;
}


/*
 * return the description for this plugin
 *
 */
string ShowNetPlugin::Description() const {
  return
"Strand ShowNet Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates a single device with 8 input and 8 output ports.\n"
"\n"
"The ports correspond to the DMX channels used in the shownet protocol.\n"
"For example the first input and output port 0 is channels 1 - 512 and\n"
"the second input and output ports are channels 513 - 1024.\n"
"\n"
"--- Config file : ola-shownet.conf ---\n"
"\n"
"ip = [a.b.c.d|<interface_name>]\n"
"The ip address or interface name to bind to. If not specified it will\n"
"use the first non-loopback interface.\n"
"\n"
"name = ola-ShowNet\n"
"The name of the node.\n";
}


/*
 * Set default preferences
 */
bool ShowNetPlugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  bool save = false;

  save |= m_preferences->SetDefaultValue(ShowNetDevice::IP_KEY,
                                         IPv4Validator(), "");
  save |= m_preferences->SetDefaultValue(SHOWNET_NAME_KEY, StringValidator(),
                                         SHOWNET_NODE_NAME);

  if (save)
    m_preferences->Save();

  if (m_preferences->GetValue(SHOWNET_NAME_KEY).empty())
    return false;
  return true;
}
}  // shownet
}  // plugin
}  // ola
