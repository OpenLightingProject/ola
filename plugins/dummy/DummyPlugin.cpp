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
#include "plugins/dummy/DummyPort.h"
#include "plugins/dummy/DummyPlugin.h"

namespace ola {
namespace plugin {
namespace dummy {

using std::string;

const char DummyPlugin::ACK_TIMER_COUNT_KEY[] = "ack_timer_count";
const char DummyPlugin::ADVANCED_DIMMER_KEY[] = "advanced_dimmer_count";
const char DummyPlugin::DEFAULT_DEVICE_COUNT[] = "1";
// 0 for now, since the web UI doesn't handle it.
const char DummyPlugin::DEFAULT_ACK_TIMER_DEVICE_COUNT[] = "0";
const char DummyPlugin::DEFAULT_SUBDEVICE_COUNT[] = "4";
const char DummyPlugin::DEVICE_NAME[] = "Dummy Device";
const char DummyPlugin::DIMMER_COUNT_KEY[] = "dimmer_count";
const char DummyPlugin::DIMMER_SUBDEVICE_COUNT_KEY[] = "dimmer_subdevice_count";
const char DummyPlugin::DUMMY_DEVICE_COUNT_KEY[] = "dummy_device_count";
const char DummyPlugin::MOVING_LIGHT_COUNT_KEY[] = "moving_light_count";
const char DummyPlugin::NETWORK_COUNT_KEY[] = "network_device_count";
const char DummyPlugin::PLUGIN_NAME[] = "Dummy";
const char DummyPlugin::PLUGIN_PREFIX[] = "dummy";
const char DummyPlugin::SENSOR_COUNT_KEY[] = "sensor_device_count";

/*
 * Start the plugin
 *
 * Lets keep it simple, one device for this plugin.
 */
bool DummyPlugin::StartHook() {
  DummyPort::Options options;

  if (!StringToInt(m_preferences->GetValue(DUMMY_DEVICE_COUNT_KEY) ,
                   &options.number_of_dummy_responders))
    StringToInt(DEFAULT_DEVICE_COUNT, &options.number_of_dummy_responders);

  if (!StringToInt(m_preferences->GetValue(DIMMER_COUNT_KEY) ,
                   &options.number_of_dimmers))
    StringToInt(DEFAULT_DEVICE_COUNT, &options.number_of_dimmers);

  if (!StringToInt(m_preferences->GetValue(DIMMER_SUBDEVICE_COUNT_KEY) ,
                   &options.dimmer_sub_device_count))
    StringToInt(DEFAULT_SUBDEVICE_COUNT, &options.dimmer_sub_device_count);

  if (!StringToInt(m_preferences->GetValue(MOVING_LIGHT_COUNT_KEY) ,
                   &options.number_of_moving_lights))
    StringToInt(DEFAULT_DEVICE_COUNT, &options.number_of_moving_lights);

  if (!StringToInt(m_preferences->GetValue(ACK_TIMER_COUNT_KEY) ,
                   &options.number_of_ack_timer_responders))
    StringToInt(DEFAULT_ACK_TIMER_DEVICE_COUNT,
                &options.number_of_ack_timer_responders);

  if (!StringToInt(m_preferences->GetValue(ADVANCED_DIMMER_KEY) ,
                   &options.number_of_advanced_dimmers))
    StringToInt(DEFAULT_DEVICE_COUNT,
                &options.number_of_advanced_dimmers);

  if (!StringToInt(m_preferences->GetValue(SENSOR_COUNT_KEY) ,
                   &options.number_of_sensor_responders))
    StringToInt(DEFAULT_DEVICE_COUNT,
                &options.number_of_sensor_responders);

  if (!StringToInt(m_preferences->GetValue(NETWORK_COUNT_KEY) ,
                   &options.number_of_network_responders))
    StringToInt(DEFAULT_DEVICE_COUNT,
                &options.number_of_network_responders);

  m_device = new DummyDevice(this, DEVICE_NAME, options);
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
"port it prints the first two bytes of dmx data to stdout.\n"
"\n"
"The Dummy plugin can also emulate a range of RDM devices. It supports the\n"
"following RDM device types:\n"
" * Dummy Device (original)\n"
" * Dimmer Rack, with a configurable number of sub-devices\n"
" * Moving Light\n"
"\n"
"The number of each device is configurable.\n"
"\n"
"--- Config file : ola-dummy.conf ---\n"
"\n"
"ack_timer_count = 0\n"
"The number of ack timer responders to create.\n"
"\n"
"advanced_dimmer_count = 0\n"
"The number of E1.37-1 dimmer responders to create.\n"
"\n"
"dimmer_count = 1\n"
"The number of dimmer devices to create.\n"
"\n"
"dimmer_subdevice_count = 1\n"
"The number of sub-devices each dimmer device should have.\n"
"\n"
"dummy_device_count = 1\n"
"The number of dummy devices to create.\n"
"\n"
"moving_light_count = 1\n"
"The number of moving light devices to create.\n"
"\n"
"sensor_device_count = 1\n"
"The number of sensor-only devices to create.\n"
"network_device_count = 1\n"
"The number of network E1.37-2 devices to create.\n"
"\n";
}


/**
 * Set the default preferences for the dummy plugin.
 */
bool DummyPlugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  bool save = false;

  save |= m_preferences->SetDefaultValue(DUMMY_DEVICE_COUNT_KEY,
                                         IntValidator(0, 254),
                                         DEFAULT_DEVICE_COUNT);

  save |= m_preferences->SetDefaultValue(DIMMER_COUNT_KEY,
                                         IntValidator(0, 254),
                                         DEFAULT_DEVICE_COUNT);

  save |= m_preferences->SetDefaultValue(DIMMER_SUBDEVICE_COUNT_KEY,
                                         IntValidator(0, 255),
                                         DEFAULT_SUBDEVICE_COUNT);

  save |= m_preferences->SetDefaultValue(MOVING_LIGHT_COUNT_KEY,
                                         IntValidator(0, 254),
                                         DEFAULT_DEVICE_COUNT);

  save |= m_preferences->SetDefaultValue(ACK_TIMER_COUNT_KEY,
                                         IntValidator(0, 254),
                                         DEFAULT_ACK_TIMER_DEVICE_COUNT);

  save |= m_preferences->SetDefaultValue(ADVANCED_DIMMER_KEY,
                                         IntValidator(0, 254),
                                         DEFAULT_DEVICE_COUNT);

  save |= m_preferences->SetDefaultValue(SENSOR_COUNT_KEY,
                                         IntValidator(0, 254),
                                         DEFAULT_DEVICE_COUNT);

  save |= m_preferences->SetDefaultValue(NETWORK_COUNT_KEY,
                                         IntValidator(0, 254),
                                         DEFAULT_DEVICE_COUNT);

  if (save)
    m_preferences->Save();

  return true;
}
}  // namespace dummy
}  // namespace plugin
}  // namespace ola
