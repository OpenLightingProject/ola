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
 * OPCDevice.cpp
 * The Open Pixel Control Device.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/openpixelcontrol/OPCDevice.h"

#include <sstream>
#include <set>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "olad/Preferences.h"
#include "plugins/openpixelcontrol/OPCPort.h"

namespace ola {
namespace plugin {
namespace openpixelcontrol {

using ola::AbstractPlugin;
using std::ostringstream;
using std::set;
using std::string;
using std::vector;

namespace {

set<uint8_t> DeDupChannels(const vector<string> &channels) {
  set<uint8_t> output;

  vector<string>::const_iterator iter = channels.begin();
  for (; iter != channels.end(); ++iter) {
    uint8_t channel;
    if (!StringToInt(*iter, &channel)) {
      OLA_WARN << "Invalid Open Pixel Control channel " << *iter;
      continue;
    }
    output.insert(channel);
  }
  return output;
}
}  // namespace

OPCServerDevice::OPCServerDevice(
    AbstractPlugin *owner,
    PluginAdaptor *plugin_adaptor,
    Preferences *preferences,
    const ola::network::IPV4SocketAddress listen_addr)
    : Device(owner, "OPC Server: " + listen_addr.ToString()),
      m_plugin_adaptor(plugin_adaptor),
      m_preferences(preferences),
      m_listen_addr(listen_addr),
      m_server(new OPCServer(plugin_adaptor, listen_addr)) {
}

string OPCServerDevice::DeviceId() const {
  return m_listen_addr.ToString();
}

bool OPCServerDevice::StartHook() {
  if (!m_server->Init()) {
    return false;
  }

  ostringstream str;
  str << "listen_" << m_listen_addr << "_channel";
  set<uint8_t> channels = DeDupChannels(
      m_preferences->GetMultipleValue(str.str()));
  set<uint8_t>::const_iterator iter = channels.begin();
  for (; iter != channels.end(); ++iter) {
    OPCInputPort *port = new OPCInputPort(this, *iter, m_plugin_adaptor,
                                          m_server.get());
    AddPort(port);
  }
  return true;
}

OPCClientDevice::OPCClientDevice(AbstractPlugin *owner,
                                 PluginAdaptor *plugin_adaptor,
                                 Preferences *preferences,
                                 const ola::network::IPV4SocketAddress target)
    : Device(owner, "OPC Client " + target.ToString()),
      m_plugin_adaptor(plugin_adaptor),
      m_preferences(preferences),
      m_target(target),
      m_client(new OPCClient(plugin_adaptor, target)) {
}

string OPCClientDevice::DeviceId() const {
  return m_target.ToString();
}

bool OPCClientDevice::StartHook() {
  ostringstream str;
  str << "target_" << m_target << "_channel";
  set<uint8_t> channels = DeDupChannels(
      m_preferences->GetMultipleValue(str.str()));
  set<uint8_t>::const_iterator iter = channels.begin();
  for (; iter != channels.end(); ++iter) {
    OPCOutputPort *port = new OPCOutputPort(this, *iter, m_client.get());
    AddPort(port);
  }
  return true;
}
}  // namespace openpixelcontrol
}  // namespace plugin
}  // namespace ola
