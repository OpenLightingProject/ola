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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * OscPlugin.cpp
 * The OSC plugin for ola. This creates a single OSC device.
 * Copyright (C) 2012 Simon Newton
 */

#define __STDC_LIMIT_MACROS  // for UINT8_MAX & friends
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ola/StringUtils.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/osc/OscAddressTemplate.h"
#include "plugins/osc/OscDevice.h"
#include "plugins/osc/OscPlugin.h"
#include "plugins/osc/OscPluginDescription.h"

namespace ola {
namespace plugin {
namespace osc {

using ola::network::IPV4SocketAddress;
using std::set;
using std::string;
using std::vector;

const char OscPlugin::DEFAULT_ADDRESS_TEMPLATE[] = "/dmx/universe/%d";
const char OscPlugin::DEFAULT_TARGETS_TEMPLATE[] = "";
const char OscPlugin::INPUT_PORT_COUNT_KEY[] = "input_ports";
const char OscPlugin::OUTPUT_PORT_COUNT_KEY[] = "output_ports";
const char OscPlugin::PLUGIN_NAME[] = "OSC";
const char OscPlugin::PLUGIN_PREFIX[] = "osc";
const char OscPlugin::PORT_ADDRESS_TEMPLATE[] = "port_%d_address";
const char OscPlugin::PORT_TARGETS_TEMPLATE[] = "port_%d_targets";
const char OscPlugin::PORT_FORMAT_TEMPLATE[] = "port_%d_output_format";
const char OscPlugin::UDP_PORT_KEY[] = "udp_listen_port";

const char OscPlugin::BLOB_FORMAT[] = "blob";
const char OscPlugin::FLOAT_ARRAY_FORMAT[] = "float_array";
const char OscPlugin::FLOAT_INDIVIDUAL_FORMAT[] = "individual_float";
const char OscPlugin::INT_ARRAY_FORMAT[] = "int_array";
const char OscPlugin::INT_INDIVIDUAL_FORMAT[] = "individual_int";

/*
 * Start the plugin.
 */
bool OscPlugin::StartHook() {
  // Get the value of UDP_PORT_KEY or use the default value if it isn't valid.
  uint16_t udp_port = StringToIntOrDefault(
      m_preferences->GetValue(UDP_PORT_KEY),
      DEFAULT_UDP_PORT);

  // For each input port, add the address to the vector
  vector<string> port_addresses;
  for (unsigned int i = 0; i < GetPortCount(INPUT_PORT_COUNT_KEY); i++) {
    const string key = ExpandTemplate(PORT_ADDRESS_TEMPLATE, i);
    port_addresses.push_back(m_preferences->GetValue(key));
  }

  // For each ouput port, extract the list of OscTargets and store them in
  // port_targets.
  OscDevice::PortConfigs port_configs;
  for (unsigned int i = 0; i < GetPortCount(OUTPUT_PORT_COUNT_KEY); i++) {
    OscDevice::PortConfig port_config;

    const string format_key = ExpandTemplate(PORT_FORMAT_TEMPLATE, i);
    SetDataFormat(m_preferences->GetValue(format_key), &port_config);

    const string key = ExpandTemplate(PORT_TARGETS_TEMPLATE, i);
    vector<string> tokens;
    StringSplit(m_preferences->GetValue(key), &tokens, ",");

    vector<string>::const_iterator iter = tokens.begin();
    for (; iter != tokens.end(); ++iter) {
      OscTarget target;
      if (ExtractOscTarget(*iter, &target)) {
        port_config.targets.push_back(target);
      }
    }

    port_configs.push_back(port_config);
  }

  // Finally create the new OscDevice, start it and register the device.
  std::auto_ptr<OscDevice> device(
    new OscDevice(this, m_plugin_adaptor, udp_port, port_addresses,
                  port_configs));
  if (!device->Start()) {
    return false;
  }
  m_device = device.release();
  m_plugin_adaptor->RegisterDevice(m_device);
  return true;
}


/*
 * Stop the plugin
 * @return true on success, false on failure
 */
bool OscPlugin::StopHook() {
  if (m_device) {
    m_plugin_adaptor->UnregisterDevice(m_device);
    bool ret = m_device->Stop();
    delete m_device;
    return ret;
  }
  return true;
}


string OscPlugin::Description() const {
  return plugin_description;
}


/**
 * Set the default preferences for the OSC plugin.
 */
bool OscPlugin::SetDefaultPreferences() {
  if (!m_preferences) {
    return false;
  }

  bool save = false;

  save |= m_preferences->SetDefaultValue(INPUT_PORT_COUNT_KEY,
                                         UIntValidator(0, 32),
                                         DEFAULT_PORT_COUNT);

  save |= m_preferences->SetDefaultValue(OUTPUT_PORT_COUNT_KEY,
                                         UIntValidator(0, 32),
                                         DEFAULT_PORT_COUNT);

  save |= m_preferences->SetDefaultValue(UDP_PORT_KEY,
                                         UIntValidator(1, UINT16_MAX),
                                         DEFAULT_UDP_PORT);

  for (unsigned int i = 0; i < GetPortCount(INPUT_PORT_COUNT_KEY); i++) {
    const string key = ExpandTemplate(PORT_ADDRESS_TEMPLATE, i);
    save |= m_preferences->SetDefaultValue(key, StringValidator(),
                                           DEFAULT_ADDRESS_TEMPLATE);
  }

  set<string> valid_formats;
  valid_formats.insert(BLOB_FORMAT);
  valid_formats.insert(FLOAT_ARRAY_FORMAT);
  valid_formats.insert(FLOAT_INDIVIDUAL_FORMAT);
  valid_formats.insert(INT_ARRAY_FORMAT);
  valid_formats.insert(INT_INDIVIDUAL_FORMAT);

  SetValidator<string> format_validator = SetValidator<string>(valid_formats);

  for (unsigned int i = 0; i < GetPortCount(OUTPUT_PORT_COUNT_KEY); i++) {
    save |= m_preferences->SetDefaultValue(
        ExpandTemplate(PORT_TARGETS_TEMPLATE, i),
        StringValidator(true),
        DEFAULT_TARGETS_TEMPLATE);

    save |= m_preferences->SetDefaultValue(
        ExpandTemplate(PORT_FORMAT_TEMPLATE, i),
        format_validator,
        BLOB_FORMAT);
  }

  if (save) {
    m_preferences->Save();
  }

  return true;
}


/**
 * @brief Given a key, return the port count from the preferences.
 *
 * Defaults to DEFAULT_PORT_COUNT if the value was invalid.
 */
unsigned int OscPlugin::GetPortCount(const string& key) const {
  unsigned int port_count;
  if (!StringToInt(m_preferences->GetValue(key), &port_count)) {
    return DEFAULT_PORT_COUNT;
  }
  return port_count;
}


/**
 * Try to parse the string as an OSC Target.
 */
bool OscPlugin::ExtractOscTarget(const string &str,
                                 OscTarget *target) {
  size_t pos = str.find_first_of("/");
  if (pos == string::npos) {
    return false;
  }

  if (!IPV4SocketAddress::FromString(str.substr(0, pos),
                                     &target->socket_address)) {
    return false;
  }
  target->osc_address = str.substr(pos);
  return true;
}


/**
 * Set the PortConfig data format based on the option the user provides
 */
void OscPlugin::SetDataFormat(const string &format_option,
                              OscDevice::PortConfig *port_config) {
  if (format_option == BLOB_FORMAT) {
    port_config->data_format = OscNode::FORMAT_BLOB;
  } else if (format_option == FLOAT_ARRAY_FORMAT) {
    port_config->data_format = OscNode::FORMAT_FLOAT_ARRAY;
  } else if (format_option == FLOAT_INDIVIDUAL_FORMAT) {
    port_config->data_format = OscNode::FORMAT_FLOAT_INDIVIDUAL;
  } else if (format_option == INT_ARRAY_FORMAT) {
    port_config->data_format = OscNode::FORMAT_INT_ARRAY;
  } else if (format_option == INT_INDIVIDUAL_FORMAT) {
    port_config->data_format = OscNode::FORMAT_INT_INDIVIDUAL;
  } else {
    OLA_WARN << "Unknown Osc format " << format_option
             << ", defaulting to blob";
  }
}
}  // namespace osc
}  // namespace plugin
}  // namespace ola
