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
 * PathportDevice.cpp
 * Pathport device
 * Copyright (C) 2005 Simon Newton
 */

#include <sstream>
#include <vector>

#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/network/NetworkUtils.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "olad/Universe.h"
#include "plugins/pathport/PathportDevice.h"
#include "plugins/pathport/PathportPort.h"
#include "plugins/pathport/PathportPlugin.h"

namespace ola {
namespace plugin {
namespace pathport {

using std::ostringstream;
using std::vector;

const char PathportDevice::K_DEFAULT_NODE_NAME[] = "ola-Pathport";
const char PathportDevice::K_DSCP_KEY[] = "dscp";
const char PathportDevice::K_NODE_ID_KEY[] = "node-id";
const char PathportDevice::K_NODE_IP_KEY[] = "ip";
const char PathportDevice::K_NODE_NAME_KEY[] = "name";
const char PathportDevice::PATHPORT_DEVICE_NAME[] = "Pathport";

/*
 * Create a new device
 */
PathportDevice::PathportDevice(PathportPlugin *owner,
                               Preferences *prefs,
                               PluginAdaptor *plugin_adaptor)
    : Device(owner, PATHPORT_DEVICE_NAME),
      m_preferences(prefs),
      m_plugin_adaptor(plugin_adaptor),
      m_node(NULL),
      m_timeout_id(ola::thread::INVALID_TIMEOUT) {
}


/*
 * Start this device
 */
bool PathportDevice::StartHook() {
  uint32_t product_id;
  if (!StringToInt(m_preferences->GetValue(K_NODE_ID_KEY), &product_id)) {
    OLA_WARN << "Invalid node Id " << m_preferences->GetValue(K_NODE_ID_KEY);
  }

  unsigned int dscp;
  if (!StringToInt(m_preferences->GetValue(K_DSCP_KEY), &dscp)) {
    OLA_WARN << "Can't convert dscp value " <<
      m_preferences->GetValue(K_DSCP_KEY) << " to int";
    dscp = 0;
  } else {
    // shift 2 bits left
    dscp = dscp << 2;
  }

  m_node = new PathportNode(m_preferences->GetValue(K_NODE_IP_KEY),
                            product_id, dscp);

  if (!m_node->Start()) {
    delete m_node;
    m_node = NULL;
    return false;
  }

  ostringstream str;
  str << PATHPORT_DEVICE_NAME << " [" << m_node->GetInterface().ip_address <<
    "]";
  SetName(str.str());

  for (unsigned int i = 0; i < PORTS_PER_DEVICE; i++) {
    PathportInputPort *port = new PathportInputPort(
        this,
        i,
        m_plugin_adaptor,
        m_node);
    AddPort(port);
  }

  for (unsigned int i = 0; i < PORTS_PER_DEVICE; i++) {
    PathportOutputPort *port = new PathportOutputPort(this, i, m_node);
    AddPort(port);
  }

  m_plugin_adaptor->AddReadDescriptor(m_node->GetSocket());
  m_timeout_id = m_plugin_adaptor->RegisterRepeatingTimeout(
      ADVERTISEMENT_PERIOD_MS,
      NewCallback(this, &PathportDevice::SendArpReply));

  return true;
}


/*
 * Stop this device
 */
void PathportDevice::PrePortStop() {
  m_plugin_adaptor->RemoveReadDescriptor(m_node->GetSocket());

  if (m_timeout_id != ola::thread::INVALID_TIMEOUT) {
    m_plugin_adaptor->RemoveTimeout(m_timeout_id);
    m_timeout_id = ola::thread::INVALID_TIMEOUT;
  }
}


/*
 * Stop this device
 */
void PathportDevice::PostPortStop() {
  m_node->Stop();
  delete m_node;
}


bool PathportDevice::SendArpReply() {
  OLA_DEBUG << "Sending pathport arp reply";
  if (m_node)
    m_node->SendArpReply();
  return true;
}
}  // namespace pathport
}  // namespace plugin
}  // namespace ola

