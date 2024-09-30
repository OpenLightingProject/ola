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
 * E133Plugin.cpp
 * The E1.33 plugin for ola
 * Copyright (C) 2024 Peter Newman
 */

#include <set>
#include <string>

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SocketAddress.h"
#include "ola/StringUtils.h"
#include "ola/acn/CID.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/e133/E133Device.h"
#include "plugins/e133/E133Plugin.h"
#include "plugins/e133/E133PluginDescription.h"

namespace ola {
namespace plugin {
namespace e133 {

using ola::acn::CID;
using ola::network::IPV4SocketAddress;
using std::string;

const char E133Plugin::CID_KEY[] = "cid";
const unsigned int E133Plugin::DEFAULT_DSCP_VALUE = 0;
const char E133Plugin::DSCP_KEY[] = "dscp";
const char E133Plugin::INPUT_PORT_COUNT_KEY[] = "input_ports";
const char E133Plugin::IP_KEY[] = "ip";
const char E133Plugin::OUTPUT_PORT_COUNT_KEY[] = "output_ports";
const char E133Plugin::PLUGIN_NAME[] = "E1.33 (RDMNet)";
const char E133Plugin::PLUGIN_PREFIX[] = "e133";
const char E133Plugin::PREPEND_HOSTNAME_KEY[] = "prepend_hostname";
const char E133Plugin::TARGET_SOCKET_KEY[] = "target_socket";
const unsigned int E133Plugin::DEFAULT_PORT_COUNT = 5;


/*
 * Start the plugin
 */
bool E133Plugin::StartHook() {
  CID cid = CID::FromString(m_preferences->GetValue(CID_KEY));
  string ip_addr = m_preferences->GetValue(IP_KEY);

  E133Device::E133DeviceOptions options;
  if (m_preferences->GetValueAsBool(PREPEND_HOSTNAME_KEY)) {
    std::ostringstream str;
    str << ola::network::Hostname() << "-" << m_plugin_adaptor->InstanceName();
//    options.source_name = str.str();
  } else {
//    options.source_name = m_plugin_adaptor->InstanceName();
  }

  unsigned int dscp;
  if (!StringToInt(m_preferences->GetValue(DSCP_KEY), &dscp)) {
    OLA_WARN << "Can't convert dscp value " <<
      m_preferences->GetValue(DSCP_KEY) << " to int";
//    options.dscp = 0;
  } else {
    // shift 2 bits left
//    options.dscp = dscp << 2;
  }

  if (!StringToInt(m_preferences->GetValue(INPUT_PORT_COUNT_KEY),
                   &options.input_ports)) {
    OLA_WARN << "Invalid value for input_ports";
  }

  if (!StringToInt(m_preferences->GetValue(OUTPUT_PORT_COUNT_KEY),
                   &options.output_ports)) {
    OLA_WARN << "Invalid value for output_ports";
  }

  IPV4SocketAddress socket_address;
  if (!IPV4SocketAddress::FromString(m_preferences->GetValue(TARGET_SOCKET_KEY), &socket_address)) {
    OLA_WARN << "Invalid value for " << TARGET_SOCKET_KEY;
  }

  m_device = new E133Device(this, cid, ip_addr, socket_address, m_plugin_adaptor, options);

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
bool E133Plugin::StopHook() {
  if (m_device) {
    m_plugin_adaptor->UnregisterDevice(m_device);
    bool ret = m_device->Stop();
    delete m_device;
    return ret;
  }
  return true;
}


/*
 * Return the description for this plugin
 */
string E133Plugin::Description() const {
    return plugin_description;
}


/*
 * Load the plugin prefs and default to sensible values
 *
 */
bool E133Plugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  bool save = false;

  CID cid = CID::FromString(m_preferences->GetValue(CID_KEY));
  if (cid.IsNil()) {
    cid = CID::Generate();
    m_preferences->SetValue(CID_KEY, cid.ToString());
    save = true;
  }

  save |= m_preferences->SetDefaultValue(
      DSCP_KEY,
      UIntValidator(0, 63),
      DEFAULT_DSCP_VALUE);

  save |= m_preferences->SetDefaultValue(
      INPUT_PORT_COUNT_KEY,
      UIntValidator(0, 512),
      DEFAULT_PORT_COUNT);

  save |= m_preferences->SetDefaultValue(
      OUTPUT_PORT_COUNT_KEY,
      UIntValidator(0, 512),
      DEFAULT_PORT_COUNT);

  save |= m_preferences->SetDefaultValue(IP_KEY, StringValidator(true), "");

  save |= m_preferences->SetDefaultValue(
      PREPEND_HOSTNAME_KEY,
      BoolValidator(),
      true);

  IPV4SocketAddress socket_address;
  if (!IPV4SocketAddress::FromString(m_preferences->GetValue(TARGET_SOCKET_KEY), &socket_address)) {
    m_preferences->SetValue(TARGET_SOCKET_KEY, "");
    save = true;
  }

  if (save) {
    m_preferences->Save();
  }

  return true;
}
}  // namespace e133
}  // namespace plugin
}  // namespace ola
