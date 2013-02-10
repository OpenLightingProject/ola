/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * DummyPlugin.cpp
 * The Dummy plugin for ola, contains a single dummy device
 * Copyright (C) 2005-2007 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "ola/StringUtils.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/dummy/DummyDevice.h"
#include "plugins/dummy/DummyPlugin.h"

namespace ola {
namespace plugin {
namespace dummy {

using std::string;

const char DummyPlugin::PLUGIN_NAME[] = "Dummy";
const char DummyPlugin::PLUGIN_PREFIX[] = "dummy";
const char DummyPlugin::DEVICE_NAME[] = "Dummy Device";
const char DummyPlugin::DEVICE_COUNT_KEY[] = "number_of_devices";
const char DummyPlugin::SUBDEVICE_COUNT_KEY[] = "number_of_subdevices";
const char DummyPlugin::DEFAULT_DEVICE_COUNT[] = "5";
const char DummyPlugin::DEFAULT_SUBDEVICE_COUNT[] = "0";

/*
 * Start the plugin
 *
 * Lets keep it simple, one device for this plugin
 */
bool DummyPlugin::StartHook() {
  unsigned int device_count, subdevice_count;
  if (!StringToInt(m_preferences->GetValue(DEVICE_COUNT_KEY) ,
                   &device_count))
    StringToInt(DEFAULT_DEVICE_COUNT, &device_count);

  if (!StringToInt(m_preferences->GetValue(SUBDEVICE_COUNT_KEY) ,
                   &subdevice_count))
    StringToInt(DEFAULT_SUBDEVICE_COUNT, &subdevice_count);

  m_device = new DummyDevice(this, DEVICE_NAME, device_count, subdevice_count);
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
"address can be changed.\n"
"--- Config file : ola-dummy.conf ---\n"
"\n"
"number_of_devices = 1\n"
"The number of fake RDM devices to create.\n"
"\n"
"number_of_subdevices = 1\n"
"The number of sub-devices each RDM device should have.\n"
"\n";
}


/**
 * Set the default preferences for the dummy plugin.
 */
bool DummyPlugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  bool save = false;

  save |= m_preferences->SetDefaultValue(DEVICE_COUNT_KEY,
                                         IntValidator(0, 254),
                                         DEFAULT_DEVICE_COUNT);

  save |= m_preferences->SetDefaultValue(SUBDEVICE_COUNT_KEY,
                                         IntValidator(0, 255),
                                         DEFAULT_SUBDEVICE_COUNT);

  if (save)
    m_preferences->Save();

  return true;
}
}  // dummy
}  // plugin
}  // ola
