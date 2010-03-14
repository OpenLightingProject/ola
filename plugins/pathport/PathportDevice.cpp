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
 * PathportDevice.cpp
 * Pathport device
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "olad/Universe.h"
#include "plugins/pathport/PathportDevice.h"
#include "plugins/pathport/PathportPort.h"
#include "plugins/pathport/PathportPlugin.h"

namespace ola {
namespace plugin {
namespace pathport {


const char PathportDevice::K_DEFAULT_NODE_NAME[] = "ola-Pathport";
const char PathportDevice::K_DSCP_KEY[] = "dscp";
const char PathportDevice::K_NODE_ID_KEY[] = "node-id";
const char PathportDevice::K_NODE_IP_KEY[] = "ip";
const char PathportDevice::K_NODE_NAME_KEY[] = "name";

/*
 * Create a new device
 */
PathportDevice::PathportDevice(PathportPlugin *owner,
                               const string &name,
                               Preferences *prefs,
                               const PluginAdaptor *plugin_adaptor)
    : Device(owner, name),
      m_preferences(prefs),
      m_plugin_adaptor(plugin_adaptor),
      m_node(NULL),
      m_enabled(false),
      m_timeout_id(ola::network::INVALID_TIMEOUT) {
}


/*
 * Start this device
 */
bool PathportDevice::Start() {
  vector<ola::network::UdpSocket*> sockets;
  vector<ola::network::UdpSocket*>::iterator iter;

  if (m_enabled)
    return false;

  uint32_t product_id;
  if (!StringToUInt(m_preferences->GetValue(K_NODE_ID_KEY), &product_id)) {
    OLA_WARN << "Invalid node Id " << m_preferences->GetValue(K_NODE_ID_KEY);
  }

  unsigned int dscp;
  if (!StringToUInt(m_preferences->GetValue(K_DSCP_KEY), &dscp)) {
    OLA_WARN << "Can't convert dscp value " <<
      m_preferences->GetValue(K_DSCP_KEY) << " to int";
    dscp = 0;
  } else {
    // shift 2 bits left
    dscp = dscp << 2;
  }

  m_node = new PathportNode(m_preferences->GetValue(K_NODE_ID_KEY),
                            product_id, dscp);

  if (!m_node->Start())
    goto e_pathport_start;

  for (unsigned int i = 0; i < PORTS_PER_DEVICE; i++) {
    PathportInputPort *port = new PathportInputPort(
        this,
        i,
        m_plugin_adaptor->WakeUpTime(),
        m_node);
    AddPort(port);
  }

  for (unsigned int i = 0; i < PORTS_PER_DEVICE; i++) {
    PathportOutputPort *port = new PathportOutputPort(this, i, m_node);
    AddPort(port);
  }

  m_plugin_adaptor->AddSocket(m_node->GetSocket());
  m_timeout_id = m_plugin_adaptor->RegisterRepeatingTimeout(
      ADVERTISTMENT_PERIOD_MS,
      NewClosure(this, &PathportDevice::SendArpReply));

  m_enabled = true;
  return true;

e_pathport_start:
  delete m_node;
  m_node = NULL;
  return false;
}


/*
 * Stop this device
 */
bool PathportDevice::Stop() {
  if (!m_enabled)
    return false;

  m_plugin_adaptor->RemoveSocket(m_node->GetSocket());

  if (m_timeout_id != ola::network::INVALID_TIMEOUT) {
    m_plugin_adaptor->RemoveTimeout(m_timeout_id);
    m_timeout_id = ola::network::INVALID_TIMEOUT;
  }

  DeleteAllPorts();
  m_node->Stop();
  delete m_node;
  m_enabled = false;
  m_node = NULL;
  return true;
}


int PathportDevice::SendArpReply() {
  OLA_DEBUG << "Sending pathport arp reply";
  if (m_node)
    m_node->SendArpReply();
  return 0;
}
}  // pathport
}  // plugin
}  // ola

