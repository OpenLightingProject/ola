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
 * E131Plugin.cpp
 * The E1.31 plugin for ola
 * Copyright (C) 2007-2009 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>

#include <olad/PluginAdaptor.h>
#include <olad/Preferences.h>

#include "E131Plugin.h"
#include "E131Device.h"


/*
 * Entry point to this plugin
 */
extern "C" ola::AbstractPlugin* create(const ola::PluginAdaptor *adaptor) {
  return new ola::e131::E131Plugin(adaptor);
}

/*
 * Called when the plugin is unloaded
 */
extern "C" void destroy(ola::Plugin* plugin) {
  delete plugin;
}


namespace ola {
namespace e131 {

const string E131Plugin::PLUGIN_NAME = "E131 Plugin";
const string E131Plugin::PLUGIN_PREFIX = "e131";

/*
 * Start the plugin
 */
bool E131Plugin::StartHook() {
  m_device = new E131Device(this, "E131 Device", m_preferences,
                            m_plugin_adaptor);

  if (!m_device->Start()) {
    delete m_device;
    return false;
  }

  m_plugin_adaptor->RegisterDevice(m_device);
  return true;
}


/*
 * Stop the plugin
 */
bool E131Plugin::StopHook() {
  m_plugin_adaptor->UnregisterDevice(m_device);
  if (!m_device->Stop())
    return false;

  delete m_device;
  return true;
}


/*
 * Return the description for this plugin
 */
string E131Plugin::Description() const {
    return
"E131 Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates a single device with eight input and eight output ports.\n"
"\n"
"Each port can be assigned to a diffent E1.31 Universe.\n"
"\n"
"--- Config file : ola-e131.conf ---\n"
"\n"
"ip = a.b.c.d\n"
"The local ip address to use for multicasting.\n"
"\n";
}

} // e131
} // ola
