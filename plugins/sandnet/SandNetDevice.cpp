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
 * SandNetDevice.cpp
 * SandNet device
 * Copyright (C) 2005 Simon Newton
 */

#include <sstream>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/sandnet/SandNetDevice.h"
#include "plugins/sandnet/SandNetPlugin.h"
#include "plugins/sandnet/SandNetPort.h"

namespace ola {
namespace plugin {
namespace sandnet {

using std::ostringstream;
using std::string;
using std::vector;

const char SandNetDevice::IP_KEY[] = "ip";
const char SandNetDevice::NAME_KEY[] = "name";
const char SandNetDevice::SANDNET_DEVICE_NAME[] = "SandNet";

/*
 * Create a new device
 */
SandNetDevice::SandNetDevice(SandNetPlugin *owner,
                             Preferences *prefs,
                             PluginAdaptor *plugin_adaptor):
  Device(owner, SANDNET_DEVICE_NAME),
  m_preferences(prefs),
  m_plugin_adaptor(plugin_adaptor),
  m_node(NULL),
  m_timeout_id(ola::thread::INVALID_TIMEOUT) {
}


/*
 * Start this device
 */
bool SandNetDevice::StartHook() {
  vector<ola::network::UDPSocket*> sockets;
  vector<ola::network::UDPSocket*>::iterator iter;

  m_node = new SandNetNode(m_preferences->GetValue(IP_KEY));
  m_node->SetName(m_preferences->GetValue(NAME_KEY));

  // setup the output ports (ie INTO sandnet)
  for (int i = 0; i < SANDNET_MAX_PORTS; i++) {
    bool ret = m_node->SetPortParameters(i,
                                         SandNetNode::SANDNET_PORT_MODE_IN,
                                         0,
                                         i);
    if (!ret) {
      OLA_WARN << "SetPortParameters failed";
      DeleteAllPorts();
      delete m_node;
      return false;
    }
  }

  if (!m_node->Start()) {
    DeleteAllPorts();
    delete m_node;
    return false;
  }

  ostringstream str;
  str << SANDNET_DEVICE_NAME << " [" << m_node->GetInterface().ip_address <<
    "]";
  SetName(str.str());


  for (unsigned int i = 0; i < INPUT_PORTS; i++) {
    SandNetInputPort *port = new SandNetInputPort(
        this,
        i,
        m_plugin_adaptor,
        m_node);
    AddPort(port);
  }
  for (unsigned int i = 0; i < SANDNET_MAX_PORTS ; i++) {
    SandNetOutputPort *port = new SandNetOutputPort(this, i, m_node);
    AddPort(port);
  }

  sockets = m_node->GetSockets();
  for (iter = sockets.begin(); iter != sockets.end(); ++iter)
    m_plugin_adaptor->AddReadDescriptor(*iter);

  m_timeout_id = m_plugin_adaptor->RegisterRepeatingTimeout(
      ADVERTISEMENT_PERIOD_MS,
      NewCallback(this, &SandNetDevice::SendAdvertisement));

  return true;
}


/*
 * Stop this device
 */
void SandNetDevice::PrePortStop() {
  vector<ola::network::UDPSocket*> sockets = m_node->GetSockets();
  vector<ola::network::UDPSocket*>::iterator iter;
  for (iter = sockets.begin(); iter != sockets.end(); ++iter)
    m_plugin_adaptor->RemoveReadDescriptor(*iter);

  if (m_timeout_id != ola::thread::INVALID_TIMEOUT) {
    m_plugin_adaptor->RemoveTimeout(m_timeout_id);
    m_timeout_id = ola::thread::INVALID_TIMEOUT;
  }
}


/*
 * Stop this device
 */
void SandNetDevice::PostPortStop() {
  m_node->Stop();
  delete m_node;
  m_node = NULL;
}


/*
 * Called periodically to send advertisements.
 */
bool SandNetDevice::SendAdvertisement() {
  OLA_DEBUG << "Sending Sandnet advertisement";
  m_node->SendAdvertisement();
  return true;
}
}  // namespace sandnet
}  // namespace plugin
}  // namespace ola
