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
 * NanoleafDevice.cpp
 * A Nanoleaf Device.
 * Copyright (C) 2017 Peter Newman
 */

#include <memory>
#include <limits>
#include <set>
#include <string>
#include <vector>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SocketAddress.h"
#include "olad/PluginAdaptor.h"
#include "olad/Port.h"
#include "olad/Preferences.h"
#include "plugins/nanoleaf/NanoleafDevice.h"
#include "plugins/nanoleaf/NanoleafPort.h"

namespace ola {
namespace plugin {
namespace nanoleaf {

using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using std::auto_ptr;
using std::set;
using std::string;
using std::vector;

const char NanoleafDevice::VERSION_V1_TEXT[] = "v1";
const char NanoleafDevice::VERSION_V2_TEXT[] = "v2";

/*
 * Create a new Nanoleaf Device
 */
NanoleafDevice::NanoleafDevice(
    AbstractPlugin *owner,
    Preferences *preferences,
    PluginAdaptor *plugin_adaptor,
    const ola::network::IPV4Address &controller)
    : Device(owner, "Nanoleaf Device"),
      m_node(NULL),
      m_preferences(preferences),
      m_plugin_adaptor(plugin_adaptor),
      m_controller(controller) {
  SetDefaults();
}


/*
 * Start this device
 * @return true on success, false on failure
 */
bool NanoleafDevice::StartHook() {
  string text_version = m_preferences->GetValue(VersionKey());

  NanoleafNode::NanoleafVersion version = NanoleafNode::VERSION_V1;
  if (text_version == VERSION_V1_TEXT) {
    version = NanoleafNode::VERSION_V1;
  } else if (text_version == VERSION_V2_TEXT) {
    version = NanoleafNode::VERSION_V2;
  } else {
    OLA_WARN << "Unknown Nanoleaf protocol version " << version
             << ", defaulting to v1";
  }

  vector<uint16_t> panels;
  vector<string> panel_list;
  StringSplit(m_preferences->GetValue(PanelsKey()), &panel_list, ",");
  vector<string>::const_iterator iter = panel_list.begin();
  for (; iter != panel_list.end(); ++iter) {
    if (iter->empty()) {
      continue;
    }

    // TODO(Peter): Check < 255 if version 1
    uint16_t panel;
    if (!StringToInt(*iter, &panel)) {
      OLA_WARN << "Invalid value for panel: " << *iter;
      return false;
    }
    panels.push_back(panel);
  }

  if (panels.empty()) {
    OLA_WARN << "No panels found";
    m_node = NULL;
    return false;
  }

  // TODO(Peter): Check and warn if we have more than a universe of panels,
  // possibly truncate the extra panels too

  // Don't bother passing in a source socket, let the node generate it's own
  m_node = new NanoleafNode(m_plugin_adaptor, panels, NULL, version);

  if (!m_node->Start()) {
    delete m_node;
    m_node = NULL;
    return false;
  }

  uint16_t ip_port;
  if (!StringToInt(m_preferences->GetValue(IPPortKey()), &ip_port)) {
    ip_port = DEFAULT_STREAMING_PORT;
  }
  IPV4SocketAddress socket_address = IPV4SocketAddress(m_controller, ip_port);
  AddPort(new NanoleafOutputPort(this, socket_address, m_node, 0));
  return true;
}


string NanoleafDevice::DeviceId() const {
  return m_controller.ToString();
}


string NanoleafDevice::IPPortKey() const {
  return m_controller.ToString() + "-port";
}


string NanoleafDevice::PanelsKey() const {
  return m_controller.ToString() + "-panels";
}


string NanoleafDevice::VersionKey() const {
  return m_controller.ToString() + "-version";
}


void NanoleafDevice::SetDefaults() {
  // Set device options
  m_preferences->SetDefaultValue(PanelsKey(), StringValidator(), "");

  set<string> valid_versions;
  valid_versions.insert(VERSION_V1_TEXT);
  valid_versions.insert(VERSION_V2_TEXT);

  m_preferences->SetDefaultValue(VersionKey(),
                                 SetValidator<string>(valid_versions),
                                 VERSION_V1_TEXT);

  m_preferences->SetDefaultValue(
      IPPortKey(),
      UIntValidator(1, std::numeric_limits<uint16_t>::max()),
      DEFAULT_STREAMING_PORT);
  m_preferences->Save();
}


/**
 * Stop this device. This is called before the ports are deleted
 */
void NanoleafDevice::PrePortStop() {
  m_node->Stop();
}


/*
 * Stop this device
 */
void NanoleafDevice::PostPortStop() {
  delete m_node;
  m_node = NULL;
}
}  // namespace nanoleaf
}  // namespace plugin
}  // namespace ola
