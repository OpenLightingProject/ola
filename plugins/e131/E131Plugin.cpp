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
#include <string>

#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/e131/E131Plugin.h"
#include "plugins/e131/E131Device.h"
#include "plugins/e131/e131/CID.h"


/*
 * Entry point to this plugin
 */
extern "C" ola::AbstractPlugin* create(const ola::PluginAdaptor *adaptor) {
  return new ola::plugin::e131::E131Plugin(adaptor);
}

/*
 * Called when the plugin is unloaded
 */
extern "C" void destroy(ola::Plugin* plugin) {
  delete plugin;
}


namespace ola {
namespace plugin {
namespace e131 {

const char E131Plugin::PLUGIN_NAME[] = "E1.31 (DMX over ACN) Plugin";
const char E131Plugin::PLUGIN_PREFIX[] = "e131";
const char E131Plugin::DEVICE_NAME[] = "E1.31 (DMX over ACN) Device";
const char E131Plugin::CID_KEY[] = "cid";


/*
 * Start the plugin
 */
bool E131Plugin::StartHook() {
  CID cid = CID::FromString(m_preferences->GetValue(CID_KEY));

  m_device = new E131Device(this, DEVICE_NAME, cid, m_preferences,
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
"E1.31 (Streaming DMX over ACN) Plugin\n"
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
"\n"
"cid = 00010203-0405-0607-0809-0A0B0C0D0E0F\n"
"The CID to use for this device\n"
"\n";
}


/*
 * Load the plugin prefs and default to sensible values
 *
 */
bool E131Plugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  CID cid = CID::FromString(m_preferences->GetValue(CID_KEY));
  if (cid.IsNil()) {
    cid = CID::Generate();
    m_preferences->SetValue(CID_KEY, cid.ToString());
    m_preferences->Save();
  }

  // check if this saved correctly
  // we don't want to use it if null
  if (m_preferences->GetValue(CID_KEY) == "")
    return false;

  return true;
}
}  // e131
}  // plugin
}  // ola
