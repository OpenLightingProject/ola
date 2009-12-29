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
 * SandNetDevice.cpp
 * SandNet device
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string>
#include <vector>

#include "ola/Logging.h"
#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/sandnet/SandNetPlugin.h"
#include "plugins/sandnet/SandNetDevice.h"
#include "plugins/sandnet/SandNetPort.h"

namespace ola {
namespace plugin {
namespace sandnet {

const char SandNetDevice::NAME_KEY[] = "name";
const char SandNetDevice::IP_KEY[] = "ip";

/*
 * Create a new device
 */
SandNetDevice::SandNetDevice(SandNetPlugin *owner,
                             const string &name,
                             Preferences *prefs,
                             const PluginAdaptor *plugin_adaptor):
  Device(owner, name),
  m_preferences(prefs),
  m_plugin_adaptor(plugin_adaptor),
  m_node(NULL),
  m_enabled(false),
  m_timeout_id(ola::network::INVALID_TIMEOUT) {
}


/*
 * Start this device
 */
bool SandNetDevice::Start() {
  vector<ola::network::UdpSocket*> sockets;
  vector<ola::network::UdpSocket*>::iterator iter;

  if (m_enabled)
    return false;

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
      goto e_sandnet_failed;
    }
  }

  if (!m_node->Start())
    goto e_sandnet_failed;

  for (unsigned int i = 0; i < INPUT_PORTS; i++) {
    SandNetInputPort *port = new SandNetInputPort(this, i, m_node);
    AddPort(port);
  }
  for (unsigned int i = 0; i < SANDNET_MAX_PORTS ; i++) {
    SandNetOutputPort *port = new SandNetOutputPort(this, i, m_node);
    AddPort(port);
  }

  sockets = m_node->GetSockets();
  for (iter = sockets.begin(); iter != sockets.end(); ++iter)
    m_plugin_adaptor->AddSocket(*iter);

  m_timeout_id = m_plugin_adaptor->RegisterRepeatingTimeout(
      ADVERTISTMENT_PERIOD_MS,
      NewClosure(this, &SandNetDevice::SendAdvertisement));

  m_enabled = true;
  return true;

  e_sandnet_failed:
    DeleteAllPorts();
    delete m_node;
    return false;
}


/*
 * Stop this device
 */
bool SandNetDevice::Stop() {
  if (!m_enabled)
    return false;

  vector<ola::network::UdpSocket*> sockets = m_node->GetSockets();
  vector<ola::network::UdpSocket*>::iterator iter;
  for (iter = sockets.begin(); iter != sockets.end(); ++iter)
    m_plugin_adaptor->RemoveSocket(*iter);

  if (m_timeout_id != ola::network::INVALID_TIMEOUT) {
    m_plugin_adaptor->RemoveTimeout(m_timeout_id);
    m_timeout_id = ola::network::INVALID_TIMEOUT;
  }

  DeleteAllPorts();
  m_node->Stop();
  delete m_node;
  m_node = NULL;
  m_enabled = false;
  return true;
}


/*
 * Called periodically to send advertisements.
 */
int SandNetDevice::SendAdvertisement() {
  OLA_DEBUG << "Sending Sandnet advertisement";
  m_node->SendAdvertisement();
  return 0;
}
}  // sandnet
}  // plugin
}  // ola
