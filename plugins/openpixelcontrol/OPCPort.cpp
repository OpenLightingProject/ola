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
 * OPCPort.cpp
 * The OPC Port for ola
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/openpixelcontrol/OPCPort.h"

#include <string>
#include "ola/base/Macro.h"
#include "plugins/openpixelcontrol/OPCClient.h"
#include "plugins/openpixelcontrol/OPCConstants.h"
#include "plugins/openpixelcontrol/OPCDevice.h"
#include "plugins/openpixelcontrol/OPCServer.h"

namespace ola {
namespace plugin {
namespace openpixelcontrol {

using std::string;

OPCInputPort::OPCInputPort(OPCServerDevice *parent,
                           uint8_t channel,
                           class PluginAdaptor *plugin_adaptor,
                           class OPCServer *server)
    : BasicInputPort(parent, channel, plugin_adaptor),
      m_channel(channel),
      m_server(server) {
  m_server->SetCallback(channel, NewCallback(this, &OPCInputPort::NewData));
}

void OPCInputPort::NewData(uint8_t command,
                           const uint8_t *data,
                           unsigned int length) {
  if (command != SET_PIXEL_COMMAND) {
    OLA_DEBUG << "Received an unknown OPC command: "
              << static_cast<int>(command);
    return;
  }
  m_buffer.Set(data, length);
  DmxChanged();
}

string OPCInputPort::Description() const {
  std::ostringstream str;
  str << m_server->ListenAddress() << ", Channel "
      << static_cast<int>(m_channel);
  return str.str();
}

OPCOutputPort::OPCOutputPort(OPCClientDevice *parent,
                             uint8_t channel,
                             OPCClient *client)
    : BasicOutputPort(parent, channel),
      m_client(client),
      m_channel(channel) {
}

bool OPCOutputPort::WriteDMX(const DmxBuffer &buffer,
                             OLA_UNUSED uint8_t priority) {
  return m_client->SendDmx(m_channel, buffer);
}

string OPCOutputPort::Description() const {
  std::ostringstream str;
  str << m_client->GetRemoteAddress() << ", Channel "
      << static_cast<int>(m_channel);
  return str.str();
}
}  // namespace openpixelcontrol
}  // namespace plugin
}  // namespace ola
