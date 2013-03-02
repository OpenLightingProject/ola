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
 * OSCPlugin.cpp
 * The OSC plugin for ola. This creates a single OSC device.
 * Copyright (C) 2012 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "ola/StringUtils.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/osc/OSCAddressTemplate.h"
#include "plugins/osc/OSCDevice.h"
#include "plugins/osc/OSCPlugin.h"

namespace ola {
namespace plugin {
namespace osc {

using std::string;
using std::vector;

const char OSCPlugin::DEFAULT_ADDRESS_TEMPLATE[] = "/dmx/universe/%d";
const char OSCPlugin::DEFAULT_PORT_COUNT[] = "5";
const char OSCPlugin::DEFAULT_TARGETS_TEMPLATE[] = "";
const char OSCPlugin::DEFAULT_UDP_PORT[] = "7770";
const char OSCPlugin::INPUT_PORT_COUNT_KEY[] = "input_ports";
const char OSCPlugin::OUTPUT_PORT_COUNT_KEY[] = "output_ports";
const char OSCPlugin::PLUGIN_NAME[] = "OSC";
const char OSCPlugin::PLUGIN_PREFIX[] = "osc";
const char OSCPlugin::PORT_ADDRESS_TEMPLATE[] = "port_%d_address";
const char OSCPlugin::PORT_TARGETS_TEMPLATE[] = "port_%d_targets";
const char OSCPlugin::UDP_PORT_KEY[] = "udp_listen_port";

/*
 * Start the plugin.
 */
bool OSCPlugin::StartHook() {
  // Get the value of UDP_PORT_KEY or use the default value if it isn't valid.
  uint16_t udp_port;
  if (!StringToInt(m_preferences->GetValue(UDP_PORT_KEY), &udp_port))
    StringToInt(DEFAULT_UDP_PORT, &udp_port);

  // For each input port, add the address to the vector
  vector<string> port_addresses;
  for (unsigned int i = 0; i < GetPortCount(INPUT_PORT_COUNT_KEY); i++) {
    const string key = ExpandTemplate(PORT_ADDRESS_TEMPLATE, i);
    port_addresses.push_back(m_preferences->GetValue(key));
  }

  // For each ouput port, extract the list of OSCTargets and store them in
  // port_targets.
  vector<vector<OSCTarget> > port_targets;
  for (unsigned int i = 0; i < GetPortCount(OUTPUT_PORT_COUNT_KEY); i++) {
    port_targets.push_back(vector<OSCTarget>());
    vector<OSCTarget> &targets = port_targets.back();

    const string key = ExpandTemplate(PORT_TARGETS_TEMPLATE, i);
    vector<string> tokens;
    StringSplit(m_preferences->GetValue(key), tokens, ",");

    vector<string>::const_iterator iter = tokens.begin();
    for (; iter != tokens.end(); ++iter) {
      OSCTarget target;
      if (ExtractOSCTarget(*iter, &target))
        targets.push_back(target);
    }
  }

  // Finally create the new OSCDevice, start it and register the device.
  m_device = new OSCDevice(this, m_plugin_adaptor, udp_port,
                           port_addresses, port_targets);
  m_device->Start();
  m_plugin_adaptor->RegisterDevice(m_device);
  return true;
}


/*
 * Stop the plugin
 * @return true on success, false on failure
 */
bool OSCPlugin::StopHook() {
  if (m_device) {
    m_plugin_adaptor->UnregisterDevice(m_device);
    bool ret = m_device->Stop();
    delete m_device;
    return ret;
  }
  return true;
}


string OSCPlugin::Description() const {
  return
"OSC (Open Sound Control) Plugin\n"
"--------------------------------\n"
"\n"
"This plugin allows OLA to send and receive OSC\n"
"( http://www.opensoundcontrol.org/ ) messages.\n"
"\n"
"OLA uses the blob type for transporting DMX data.\n"
"\n"
"--- Config file : ola-osc.conf ---\n"
"\n"
"input_ports = <int>\n"
"The number of input ports to create.\n"
"\n"
"output_ports = <int>\n"
"The number of output ports to create.\n"
"\n"
"port_N_targets = ip:port/address,ip:port/address,...\n"
"For output port N, the list of targets to send OSC messages to.\n"
"If the address contains %d, it's replaced by the universe number.\n"
"\n"
"port_N_address = /address\n"
"The OSC address to listen on for port N. If the address contains %d\n"
"it's replaced by the universe number for port N.\n"
"\n"
"udp_listen_port = <int>\n"
"The UDP Port to listen on for OSC messages.\n"
"\n";
}


/**
 * Set the default preferences for the OSC plugin.
 */
bool OSCPlugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  bool save = false;

  save |= m_preferences->SetDefaultValue(INPUT_PORT_COUNT_KEY,
                                         IntValidator(0, 32),
                                         DEFAULT_PORT_COUNT);

  save |= m_preferences->SetDefaultValue(OUTPUT_PORT_COUNT_KEY,
                                         IntValidator(0, 32),
                                         DEFAULT_PORT_COUNT);

  save |= m_preferences->SetDefaultValue(UDP_PORT_KEY,
                                         IntValidator(1, 0xffff),
                                         DEFAULT_UDP_PORT);

  for (unsigned int i = 0; i < GetPortCount(INPUT_PORT_COUNT_KEY); i++) {
    const string key = ExpandTemplate(PORT_ADDRESS_TEMPLATE, i);
    save |= m_preferences->SetDefaultValue(key, StringValidator(),
                                           DEFAULT_ADDRESS_TEMPLATE);
  }

  for (unsigned int i = 0; i < GetPortCount(OUTPUT_PORT_COUNT_KEY); i++) {
    const string key = ExpandTemplate(PORT_TARGETS_TEMPLATE, i);
    save |= m_preferences->SetDefaultValue(key, StringValidator(true),
                                           DEFAULT_TARGETS_TEMPLATE);
  }

  if (save)
    m_preferences->Save();

  return true;
}


/**
 * Given a key, return the port count from the preferences. Defaults to
 * DEFAULT_PORT_COUNT if the value was invalid.
 */
unsigned int OSCPlugin::GetPortCount(const string& key) const {
  unsigned int port_count;
  if (!StringToInt(m_preferences->GetValue(key), &port_count))
    StringToInt(DEFAULT_PORT_COUNT, &port_count);
  return port_count;
}


/**
 * Try to parse the string as an OSC Target.
 */
bool OSCPlugin::ExtractOSCTarget(const string &str,
                                 OSCTarget *target) {
  size_t pos = str.find_first_of("/");
  if (pos == string::npos)
    return false;

  if (!IPV4SocketAddress::FromString(str.substr(0, pos),
                                     &target->socket_address))
    return false;
  target->osc_address = str.substr(pos);
  return true;
}
}  // osc
}  // plugin
}  // ola
